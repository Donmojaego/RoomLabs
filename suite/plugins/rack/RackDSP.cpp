#include "RackDSP.h"
#include <algorithm>
#include <cmath>

namespace rl::rack
{
void remixPattern (int style, float density, std::array<int, 16>& steps)
{
    static const int W[12][16] = {
        { 0,3,3,2,1,1,1,2,1,3,1,2,0,1,2,0 }, { 0,6,5,0,0,0,0,2,1,0,0,0,0,0,1,0 }, { 0,0,0,1,4,6,0,0,1,0,1,0,1,0,0,4 },
        { 0,1,0,1,1,1,0,1,2,0,4,0,6,3,0,1 }, { 0,1,0,0,2,1,0,0,0,7,4,0,1,0,0,3 }, { 0,2,8,0,0,0,0,2,0,0,0,0,0,1,3,0 },
        { 0,1,0,8,2,0,1,0,0,0,1,0,1,0,0,1 }, { 0,1,1,2,0,1,8,0,0,0,0,0,0,1,0,1 }, { 0,0,0,0,0,0,0,8,4,0,1,0,0,3,0,0 },
        { 0,0,0,3,5,1,0,0,0,0,3,1,6,1,0,4 }, { 0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 }, { 0,2,1,1,0,0,0,1,2,0,2,0,1,0,0,0 } };
    style = std::clamp (style, 0, 11); const int* w = W[style]; int tot = 0; for (int i = 0; i < 16; ++i) tot += w[i];
    uint32_t s = 0x9E3779B9u * (uint32_t) (style + 1);
    auto rnd = [&s] { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 16777216.f; };
    for (int i = 0; i < 16; ++i)
    {
        int fx = 0; const float r1 = rnd(), r2 = rnd();
        if (r1 < density * (i % 4 == 0 ? 0.55f : 1.f)) { float r = r2 * (float) tot; for (int k = 0; k < 16; ++k) { r -= (float) w[k]; if (r < 0) { fx = k; break; } } }
        steps[(size_t) i] = fx;
    }
}

void Engine::prepare (double s, int maxBlock)
{
    sr = s;
    for (auto& v : dry) v.assign ((size_t) std::max (1, maxBlock), 0.f);
    for (auto& v : wet) v.assign ((size_t) std::max (1, maxBlock), 0.f);
    for (auto& x : st) { x.noise.prepare (sr); x.reverb.prepare (sr); x.remix.prepare (sr); }
    reset();
}

void Engine::reset()
{
    for (int k = 0; k < 3; ++k)
    {
        auto& x = st[(size_t) k]; x.noise.reset(); x.reverb.reset(); x.remix.reset();
        x.active = prm.slot[(size_t) k].module; x.g = 1; x.onS = prm.slot[(size_t) k].on ? 1.f : 0.f; x.lastStyle = -1;
    }
}

void Engine::runModule (int k, State& s, float* const* io, int numCh, int n)
{
    const Slot& p = prm.slot[(size_t) k];
    switch (s.active)
    {
        case Noise:
        {
            rl::nz::Params q; q.on = p.on; q.type = 1; q.levelDb = -60.f + 60.f * p.a; q.color = p.b * 2 - 1; q.width = p.c; q.keepInput = false;
            s.noise.setParams (q); s.noise.process (io, numCh, n); break;
        }
        case Reverb:
        {
            rl::rv::Params q; q.type = p.a < 0.34f ? 0 : p.a < 0.67f ? 1 : 2; q.size = p.a; q.decay = 0.2f * std::pow (60.f, p.b); q.dampHz = 1500.f * std::pow (12.f, p.c);
            q.mix = 1.f; q.bypass = ! p.on; s.reverb.setParams (q); s.reverb.process (io, numCh, n); break;
        }
        case Remix:
        {
            const int style = (int) std::lround (p.a * 11);
            if (style != s.lastStyle || std::abs (p.b - s.lastDens) > 0.02f) { remixPattern (style, p.b, s.steps); s.lastStyle = style; s.lastDens = p.b; }
            rl::rx::Params q; q.steps = s.steps; q.mix = 1.f; q.cutoff = 20000.f;
            q.lfo[0] = { 0, 3, p.c, 0 };
            s.remix.setParams (q); s.remix.setTransport (tBpm, tPpq, tPlay); s.remix.process (io, numCh, n); break;
        }
        default: break;
    }
}

void Engine::process (float* const* io, int numCh, int n)
{
    if (n > (int) dry[0].size()) n = (int) dry[0].size();   // garde-fou (l'hôte respecte maxBlock)
    const float inc = (float) (1.0 / (0.02 * sr));
    for (int k = 0; k < 3; ++k)
    {
        auto& s = st[(size_t) k]; const Slot& p = prm.slot[(size_t) k];
        const int ch = std::min (numCh, 2);
        for (int c = 0; c < ch; ++c) { std::copy (io[c], io[c] + n, dry[c].begin()); std::copy (io[c], io[c] + n, wet[c].begin()); }
        float* w[2] = { wet[0].data(), wet[ch > 1 ? 1 : 0].data() };
        if (s.active != Empty) runModule (k, s, w, ch, n);
        // fondu : g descend vers 0 si le module a changé, puis on bascule et on remonte
        const bool tailOff = s.active == Reverb;   // en OFF la Reverb continue de sonner (queue)
        double acc = 0;
        for (int i = 0; i < n; ++i)
        {
            const float tgtG = p.module == s.active ? 1.f : 0.f;
            s.g = tgtG > s.g ? std::min (1.f, s.g + inc) : std::max (0.f, s.g - inc);
            s.onS = p.on ? std::min (1.f, s.onS + inc) : std::max (0.f, s.onS - inc);
            const float m = s.active == Empty ? 0.f : p.mix * s.g * (tailOff ? 1.f : s.onS);
            for (int c = 0; c < ch; ++c)
            {
                const float d = dry[c][(size_t) i], e = wet[c][(size_t) i];
                float y;
                if (s.active == Reverb) y = d + e * m * 0.9f;               // effet parallèle : le direct reste entier
                else if (s.active == Noise) y = d + e * s.g * s.onS * p.mix;      // le générateur s'ajoute (sa propre sortie = bruit seul)
                else y = d * (1 - m) + e * m;
                io[c][i] = std::isfinite (y) ? y : d;
            }
            acc += (double) io[0][i] * io[0][i];
        }
        if (s.g <= 0.f && p.module != s.active)
        {
            s.active = p.module; s.lastStyle = -1;
            if (s.active == Noise) s.noise.reset(); else if (s.active == Reverb) s.reverb.reset(); else if (s.active == Remix) s.remix.reset();
        }
        meter.lvl[(size_t) k].store (n > 0 ? (float) std::sqrt (acc / n) : 0.f);
    }
    const float og = std::pow (10.f, prm.outDb / 20.f);
    double acc = 0;
    for (int c = 0; c < std::min (numCh, 2); ++c) for (int i = 0; i < n; ++i) { io[c][i] = std::clamp (io[c][i] * og, -2.f, 2.f); if (c == 0) acc += (double) io[0][i] * io[0][i]; }
    meter.out.store (n > 0 ? (float) std::sqrt (acc / n) : 0.f);
}
} // namespace rl::rack
