#include "SoupEngine.h"
#include <algorithm>
#include <cmath>

namespace soup
{
namespace
{
constexpr double twoPi = 6.283185307179586, pi = 3.141592653589793;
const double divBeats[8] = { 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
inline float t60 (double s, double sr) { return (float) std::exp (std::log (0.001) / (std::max (s, 0.0005) * sr)); }
inline float onePole (double hz, double sr) { return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr)); }
inline double polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}
inline double saw (double p, double dt) { return 2.0 * p - 1.0 - polyBlep (p, dt); }
inline float hann (double x) { return (float) (0.5 - 0.5 * std::cos (twoPi * std::clamp (x, 0.0, 1.0))); }
inline bool percussive (Inst i) { return i == Inst::Bell || i == Inst::Drum || i == Inst::Koto || i == Inst::Shamisen || i == Inst::Gamelan || i == Inst::Oud || i == Inst::Taiko; }
} // namespace

void SoupEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1000 ? sampleRate : 48000; invSr = 1.0 / sr;
    hist.assign ((size_t) (sr * 8), 0.f);
    dlL.assign ((size_t) (sr * 4), 0.f); dlR.assign ((size_t) (sr * 4), 0.f);
    for (int s = 0; s < 2; ++s) { revBuf[s].assign ((size_t) (sr * 8), 0.f); stuBuf[s].assign ((size_t) (sr * 8), 0.f); }
    static const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }, apT[4] = { 556, 441, 341, 225 };
    const double k = sr / 44100.0;
    for (int i = 0; i < 8; ++i) { combL[(size_t) i].b.assign ((size_t) (combT[i] * k) + 1, 0.f); combR[(size_t) i].b.assign ((size_t) ((combT[i] + 23) * k) + 1, 0.f); }
    for (int i = 0; i < 4; ++i) { apL[(size_t) i].b.assign ((size_t) (apT[i] * k) + 1, 0.f); apR[(size_t) i].b.assign ((size_t) ((apT[i] + 23) * k) + 1, 0.f); }
    reset();
}

void SoupEngine::reset()
{
    for (auto& v : voices) v = Voice {};
    for (auto& g : grains) g = Grain {};
    std::fill (hist.begin(), hist.end(), 0.f); hw = 0;
    std::fill (dlL.begin(), dlL.end(), 0.f); std::fill (dlR.begin(), dlR.end(), 0.f); dw = 0; dLp = dRp = 0;
    for (int s = 0; s < 2; ++s) { std::fill (revBuf[s].begin(), revBuf[s].end(), 0.f); std::fill (stuBuf[s].begin(), stuBuf[s].end(), 0.f); revW[s] = stuW[s] = 0; }
    for (auto* a : { &combL, &combR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; c.store = 0; }
    for (auto* a : { &apL, &apR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; }
    heldCount = 0; arpIdx = -1; arpPhaseDiv = -1; strRead = 0; grainClock = 0;
    fic1 = fic2 = toneLow = follower = wetFollower = 0;
}

void SoupEngine::setParams (const Params& p)
{
    const bool arpOff = prm.arpOn && ! p.arpOn;
    prm = p;
    prm.division = std::clamp (prm.division, 0, 7);
    prm.arpOct = std::clamp (prm.arpOct, 1, 3);
    if (arpOff) for (auto& v : voices) if (v.active && v.key == -1) { v.released = true; v.stage = 3; }
}

void SoupEngine::setTransport (double tempo, double ppqStart)
{
    if (tempo > 20 && tempo < 400) bpm = tempo;
    ppq = ppqStart;
}

double SoupEngine::divisionSamples() const { return divBeats[prm.division] * 60.0 / bpm * sr; }

float SoupEngine::noise() { rng = rng * 1664525u + 1013904223u; return ((float) (rng >> 9) / 4194304.f) - 1.f; }

int SoupEngine::activeVoices() const { int n = 0; for (auto& v : voices) n += v.active; return n; }

// ---------------------------------------------------------------------------
void SoupEngine::noteOn (int note, float vel)
{
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = std::min (w, (int) held.size() - 1);
    held[(size_t) heldCount++] = note;
    if (prm.arpOn) { if (heldCount == 1) { arpIdx = -1; arpPhaseDiv = -1; } return; }
    startVoice (note, note, vel);
    fb.noteHits.fetch_add (1, std::memory_order_relaxed);
}

void SoupEngine::noteOff (int note)
{
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = w;
    for (auto& v : voices)
        if (v.active && ! v.released && (v.key == note || (heldCount == 0 && v.key == -1))) { v.released = true; v.stage = 3; }
}

void SoupEngine::allNotesOff()
{
    heldCount = 0;
    for (auto& v : voices) if (v.active) { v.released = true; v.stage = 3; }
}

void SoupEngine::startVoice (int note, int key, float vel)
{
    if (note < 0 || note > 127) return;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (! v) { float best = 2; for (auto& x : voices) if (x.released && x.env < best) { best = x.env; v = &x; } }
    if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    const float keep = v->active ? v->env : 0.f;
    *v = Voice {};
    v->active = true; v->note = note; v->key = key; v->vel = std::clamp (vel, 0.f, 1.f);
    v->hz = 440.0 * std::pow (2.0, (note + prm.tune - 69.0) / 12.0);
    v->env = keep; v->age = ++ageCounter;
    v->ph[1] = 0.37; v->ph[2] = 0.71;
}

// ---------------------------------------------------------------------------
float SoupEngine::renderVoice (Voice& v)
{
    v.t += invSr;
    const bool perc = percussive (prm.inst);
    switch (v.stage)
    {
        case 0: v.env += aInc; if (v.env >= 1.f) { v.env = 1.f; v.stage = 1; } break;
        case 1: { const float sus = perc ? 0.f : prm.sustain; v.env = sus + (v.env - sus) * dCoef; break; }
        default: v.env *= rCoef; break;
    }
    if (v.env < 1e-5f && (v.stage == 3 || (perc && v.stage == 1 && v.t > 0.02))) { v.active = false; return 0.f; }

    const float br = std::clamp (prm.brightness, 0.f, 1.f);
    double hz = v.hz;
    float s = 0.f, cutoff = 4000.f, reso = 0.15f;
    auto vib = [&] (double depthCents, double delay) {
        const double d = depthCents * std::clamp ((v.t - delay) / 0.4, 0.0, 1.0);
        return std::pow (2.0, d * std::sin (twoPi * 5.4 * v.t) / 1200.0); };

    switch (prm.inst)
    {
        case Inst::Violin:
        {
            hz *= vib (16, 0.25);
            for (int u = 0; u < 2; ++u)
            {
                const double f = hz * (u ? 1.0035 : 0.9965), dt = f * invSr;
                v.ph[u] += dt; if (v.ph[u] >= 1) v.ph[u] -= 1;
                s += (float) saw (v.ph[u], dt) * 0.5f;
            }
            s += 0.04f * noise() * (float) std::exp (-v.t / 0.15);       // archet
            cutoff = 1100.f + 5200.f * br; reso = 0.35f; break;
        }
        case Inst::Pad:
        case Inst::Strings:
        {
            const bool str = prm.inst == Inst::Strings;
            if (str) hz *= vib (7, 0.4);
            for (int u = 0; u < 3; ++u)
            {
                const double det[3] = { -1.0, 0.0, 1.0 };
                const double f = hz * std::pow (2.0, det[u] * (str ? 16.0 : 11.0) / 1200.0), dt = f * invSr;
                v.ph[u] += dt; if (v.ph[u] >= 1) v.ph[u] -= 1;
                s += (float) (u == 1 && ! str ? 1.0 - 4.0 * std::abs (v.ph[u] - 0.5) : saw (v.ph[u], dt)) * 0.42f;
            }
            cutoff = (str ? 1600.f : 500.f) + (str ? 5000.f : 4200.f) * br; reso = 0.12f; break;
        }
        case Inst::Keys:
        {
            v.modEnv *= t60 (0.9, sr) < 1 ? (float) std::exp (std::log (0.001) / (0.9 * sr)) : 1.f;
            v.partEnv[0] *= (float) std::exp (std::log (0.001) / (0.12 * sr));
            v.mph += hz * invSr; if (v.mph >= 1) v.mph -= 1;
            v.ph[0] += hz * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            v.ph[1] += hz * 14.0 * invSr; if (v.ph[1] >= 1) v.ph[1] -= 1;
            s = (float) std::sin (v.ph[0] * twoPi + (1.2 + 2.3 * br) * v.modEnv * std::sin (v.mph * twoPi))
              + 0.12f * v.partEnv[0] * (float) std::sin (v.ph[1] * twoPi);
            s *= 0.7f; cutoff = 9000.f; break;
        }
        case Inst::Bell:
        {
            static const double ratio[3] = { 1.0, 2.76, 5.4 };
            static const float amp[3] = { 1.f, 0.5f, 0.25f };
            const double dec = 0.18 + 0.5 * br;
            for (int i = 0; i < 3; ++i)
            {
                const double f = hz * ratio[i];
                if (f >= sr * 0.45) continue;
                v.part[i] += f * invSr; if (v.part[i] >= 1) v.part[i] -= 1;
                v.partEnv[i] *= (float) std::exp (std::log (0.001) / (dec / (1 + i) * sr));
                s += amp[i] * v.partEnv[i] * (float) std::sin (v.part[i] * twoPi);
            }
            if (v.t < 0.004) s += 0.35f * noise() * (float) std::exp (-v.t / 0.001);   // « cloc »
            s *= 0.65f; cutoff = 14000.f; break;
        }
        case Inst::Texture:
        {
            const double dt = hz * invSr;
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            v.ph[1] += dt * 2.003; if (v.ph[1] >= 1) v.ph[1] -= 1;
            s = 0.35f * (float) saw (v.ph[0], dt) + 0.25f * (float) std::sin (v.ph[1] * twoPi) + 0.25f * noise();
            cutoff = (float) std::min (hz * (3.0 + 6.0 * br), 14000.0); reso = 0.75f; break;
        }
        case Inst::Bass:
        {
            const double dt = hz * invSr;
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            s = (float) std::sin (v.ph[0] * twoPi) * 0.8f + 0.25f * (float) (1.0 - 4.0 * std::abs (v.ph[0] - 0.5));
            cutoff = 250.f + 1100.f * br; reso = 0.2f; break;
        }
        case Inst::Koto: case Inst::Shamisen: case Inst::Oud:
        {
            // corde pincée additive : 8 harmoniques, aigus qui s'éteignent vite ;
            // koto : glissé vers la note (pression sur la corde) ; shamisen : sawari (grésillement) ; oud : deux cordes (chœur)
            const bool sham = prm.inst == Inst::Shamisen, oud = prm.inst == Inst::Oud;
            double f0 = hz;
            if (prm.inst == Inst::Koto) f0 *= std::pow (2.0, -0.35 * std::exp (-v.t / 0.03) / 12.0);
            const double dec = (sham ? 0.6 : oud ? 1.4 : 1.8) * (0.6 + 0.8 * br) * std::pow (261.6 / std::max (60.0, hz), 0.4);
            if (v.t <= invSr * 1.5) for (int k = 0; k < 8; ++k) v.he[k] = (float) (1.0 / std::pow (k + 1.0, (sham ? 0.8 : oud ? 1.4 : 1.1) - 0.3 * br));
            for (int k = 0; k < 8; ++k)
            {
                const double f = f0 * (k + 1) * (1 + 0.0004 * k * k);
                if (f >= sr * 0.45) break;
                v.hp[k] += f * invSr; if (v.hp[k] >= 1) v.hp[k] -= 1;
                v.he[k] *= (float) std::exp (std::log (0.001) / (dec / (1 + 0.6 * k) * sr));
                s += v.he[k] * (float) std::sin (v.hp[k] * twoPi);
                if (oud) s += 0.6f * v.he[k] * (float) std::sin ((v.hp[k] + 0.0021 * v.t * f) * twoPi);
            }
            if (sham) { v.buzz += 0.02f * (std::abs (s) - v.buzz); s += 0.5f * noise() * v.buzz * v.buzz; }
            if (v.t < 0.003) s += 0.4f * noise() * (float) std::exp (-v.t / 0.0008);   // attaque du plectre / de l'onglet
            s *= oud ? 0.35f : 0.45f; cutoff = (sham ? 5000.f : 3500.f) + 7000.f * br; reso = 0.1f; break;
        }
        case Inst::Erhu:
        {
            // vièle à deux cordes : arrivée glissée sur la note, vibrato large, timbre nasal
            hz *= std::pow (2.0, -1.0 * std::exp (-v.t / 0.07) / 12.0) * vib (28, 0.18);
            const double dt = hz * invSr;
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            s = (float) saw (v.ph[0], dt) * 0.7f + 0.03f * noise();
            // formants nasaux (≈ 1 kHz et 2,6 kHz)
            auto bp = [&] (int i, double fc, double q, float x) {          // filtre d'état (Chamberlin) : sortie passe-bande
                const float f = (float) (2.0 * std::sin (pi * fc * invSr)), qq = (float) (1.0 / q);
                float& low = v.bp[i][0]; float& band = v.bp[i][1];
                low += f * band; const float high = x - low - qq * band; band += f * high; return band; };
            s = 0.18f * s + 0.75f * bp (0, 1000, 3, s) + 0.45f * bp (1, 2600, 4, s);
            cutoff = 2500.f + 5000.f * br; reso = 0.15f; break;
        }
        case Inst::Shakuhachi:
        {
            // flûte de bambou : souffle très présent, attaque par en dessous, vibrato lent
            hz *= std::pow (2.0, -0.6 * std::exp (-v.t / 0.09) / 12.0) * vib (22, 0.35);
            v.ph[0] += hz * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            const float breath = noise() * (0.22f + 0.45f * (float) std::exp (-v.t / 0.12));
            s = (float) std::sin (v.ph[0] * twoPi) * 0.75f + 0.08f * (float) std::sin (2 * v.ph[0] * twoPi) + breath;
            cutoff = (float) std::min (hz * 6.0 + 1500.0 * br, 9000.0); reso = 0.3f; break;
        }
        case Inst::Gamelan:
        {
            // bonang / gong : partiels inharmoniques, deux instruments accordés en battement (ombak)
            static const double ratio[6] = { 1.0, 1.52, 2.0, 2.83, 3.95, 5.4 };
            static const float amp[6] = { 1.f, .45f, .35f, .25f, .15f, .08f };
            if (v.t <= invSr * 1.5) for (int k = 0; k < 6; ++k) v.he[k] = amp[k];
            const double dec = 1.2 + 2.5 * br;
            for (int k = 0; k < 6; ++k)
            {
                const double f = hz * ratio[k]; if (f >= sr * 0.45) break;
                v.hp[k] += f * invSr; if (v.hp[k] >= 1) v.hp[k] -= 1;
                v.he[k] *= (float) std::exp (std::log (0.001) / (dec / (1 + 0.7 * k) * sr));
                s += v.he[k] * ((float) std::sin (v.hp[k] * twoPi) + 0.8f * (float) std::sin ((v.hp[k] + 5.0 * v.t) * twoPi));
            }
            if (v.t < 0.004) s += 0.3f * noise() * (float) std::exp (-v.t / 0.001);
            s *= 0.32f; cutoff = 12000.f; break;
        }
        case Inst::Taiko:
        {
            // grand tambour : peau grave qui descend, frappe large
            const double f = hz * 0.5 * (1.0 + 0.5 * std::exp (-v.t / 0.04));
            v.ph[0] += f * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            v.ph[1] += f * 1.59 * invSr; if (v.ph[1] >= 1) v.ph[1] -= 1;
            v.partEnv[0] *= (float) std::exp (std::log (0.001) / ((0.5 + 0.6 * br) * sr));
            v.partEnv[1] *= (float) std::exp (std::log (0.001) / ((0.25 + 0.3 * br) * sr));
            s = (float) std::sin (v.ph[0] * twoPi) * v.partEnv[0] + 0.4f * (float) std::sin (v.ph[1] * twoPi) * v.partEnv[1]
              + 0.35f * noise() * (float) std::exp (-v.t / 0.012);
            s *= 0.9f; cutoff = 3000.f + 4000.f * br; break;
        }
        case Inst::Drum:
        default:
        {
            const double f = hz * (1.0 + std::exp (-v.t / 0.02));
            v.ph[0] += f * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            v.partEnv[0] *= (float) std::exp (std::log (0.001) / ((0.12 + 0.4 * br) * sr));
            s = (float) std::sin (v.ph[0] * twoPi) * v.partEnv[0] + 0.3f * noise() * (float) std::exp (-v.t / 0.006);   // « touc »
            cutoff = 6000.f; break;
        }
    }

    // passe-bas SVF (coefficients tous les 8 échantillons)
    if ((v.tick++ & 7) == 0)
    {
        const float g = (float) std::tan (pi * std::min ((double) cutoff, sr * 0.45) / sr);
        const float k = 2.f - 1.9f * reso;
        v.a1 = 1.f / (1.f + g * (g + k)); v.a2 = g * v.a1; v.a3 = g * v.a2;
    }
    const float v3 = s - v.ic2, v1 = v.a1 * v.ic1 + v.a2 * v3, v2 = v.ic2 + v.a2 * v.ic1 + v.a3 * v3;
    v.ic1 = 2 * v1 - v.ic1; v.ic2 = 2 * v2 - v.ic2;
    return v2 * v.env * (0.45f + 0.55f * v.vel);
}

// ---------------------------------------------------------------------------
// Reverse calé : chaque division rejoue la précédente à l'envers (fondus de 6 ms)
float SoupEngine::reverseStage (float x, int which)
{
    auto& b = revBuf[which]; int& w = revW[which]; const int sz = (int) b.size();
    b[(size_t) w] = x;
    const double L = std::min (divisionSamples(), sz / 2.0 - 2);
    const double dBeats = divBeats[prm.division];
    const double pos = (beatPos / dBeats - std::floor (beatPos / dBeats)) * L;
    int idx = w - (int) (2.0 * pos) - 1; while (idx < 0) idx += sz;
    const double fade = std::min (0.015 * sr, L * 0.2);
    const float g = (float) std::clamp (std::min (pos / fade, (L - pos) / fade), 0.0, 1.0);
    w = (w + 1) % sz;
    return b[(size_t) idx] * g + x * (1.f - g) * 0.5f;          // jonctions adoucies (fondu vers le son direct)
}

// Stutter (adouci, v0.1.1) : sur un cycle de 4 tranches, les 2 premières passent telles quelles,
// les 2 dernières répètent la 2e tranche. Aux jonctions, fondu enchaîné (12 ms) avec le son direct :
// plus de « trou » de silence entre les répétitions.
float SoupEngine::stutterStage (float x, int which)
{
    auto& b = stuBuf[which]; int& w = stuW[which]; const int sz = (int) b.size();
    b[(size_t) w] = x;
    const double dBeats = divBeats[prm.division];
    const double L = std::min (divisionSamples(), sz / 5.0);
    const double cycleBeats = dBeats * 4.0;
    const double p = (beatPos / cycleBeats - std::floor (beatPos / cycleBeats)) * 4.0 * L;   // position dans le cycle (échantillons)
    const int slice = std::min (3, (int) (p / L));
    w = (w + 1) % sz;
    if (slice < 2) return x;                                  // moitié du cycle : son direct
    const double inSlice = p - slice * L;
    // la 2e tranche (enregistrée il y a (slice-1)·L + inSlice échantillons… on relit son début)
    int idx = (w - 1) - (int) (p - (L + inSlice)); while (idx < 0) idx += sz; idx %= sz;
    const float rep = b[(size_t) idx];
    const double fade = std::min (0.012 * sr, L * 0.25);
    const float g = (float) std::clamp (std::min (inSlice / fade, (L - inSlice) / fade), 0.0, 1.0);
    return rep * g + x * (1.f - g);                            // jonctions : fondu avec le son direct
}

float SoupEngine::processFx (Fx type, float x, float& wetR, int)
{
    const int hs = (int) hist.size();
    const double L = divisionSamples();
    const double dBeats = divBeats[prm.division];
    switch (type)
    {
        case Fx::Reverse: { const float y = reverseStage (x, 0); wetR = y; return y; }
        case Fx::Stutter: { const float y = stutterStage (x, 0); wetR = y; return y; }
        case Fx::Repeat:
        {
            const int sz = (int) dlL.size();
            const int d = std::clamp ((int) L, 1, sz - 1);
            int r = dw - d; if (r < 0) r += sz;
            const float yl = dlL[(size_t) r], yr = dlR[(size_t) r];
            const float a = onePole (900 + 9000 * (1 - prm.damp), sr);
            dLp += a * (yr - dLp); dRp += a * (yl - dRp);          // ping-pong amorti
            const float fbk = std::clamp (prm.feedback, 0.f, 0.92f);
            dlL[(size_t) dw] = x + dLp * fbk;
            dlR[(size_t) dw] = dRp * fbk;
            dw = (dw + 1) % sz;
            wetR = x + yr; return x + yl;
        }
        case Fx::Slicer:
        {
            static const uint16_t pattern = 0b1011011010110110;
            const double steps = beatPos / dBeats;
            const int step = ((int) std::floor (steps)) & 15;
            const double within = steps - std::floor (steps);
            const bool on = (pattern >> (15 - step)) & 1;
            const float g = on ? (float) std::min ({ 1.0, within * L / (0.002 * sr), (1.0 - within) * L / (0.004 * sr) * 1.0 }) : 0.f;
            const float y = x * std::clamp (g, 0.f, 1.f);
            wetR = y; return y;
        }
        case Fx::Stretch:
        {
            // lecture granulaire plus lente que le temps réel : le son s'étire
            const double rate = 1.0 - 0.8 * prm.stretch;
            const int glen = (int) (0.11 * sr);
            strRead += rate;
            double lag = hw - strRead; if (lag < 0) lag += hs;
            if (lag > sr * 2.5 || lag < glen) strRead = hw - glen * 1.5;
            float y = 0.f;
            for (int g = 0; g < 2; ++g)
            {
                if (strGrainAge[g] <= 0 || strGrainAge[g] >= glen)
                    if ((g == 0 && (grainClock % glen) == 0) || (g == 1 && (grainClock % glen) == glen / 2))
                    { strGrainPos[g] = strRead; strGrainAge[g] = 0; }
                if (strGrainAge[g] < glen)
                {
                    double rp = strGrainPos[g] + strGrainAge[g]; while (rp < 0) rp += hs; while (rp >= hs) rp -= hs;
                    const int i0 = (int) rp; const float fr = (float) (rp - i0);
                    y += (hist[(size_t) i0] + fr * (hist[(size_t) ((i0 + 1) % hs)] - hist[(size_t) i0])) * hann ((double) strGrainAge[g] / glen);
                    ++strGrainAge[g];
                }
            }
            ++grainClock;
            wetR = y; return y;
        }
        case Fx::FilterMod:
        {
            const double ph = beatPos / (dBeats * 4.0);
            const double lfo = 0.5 + 0.5 * std::sin (twoPi * (ph - std::floor (ph)));
            const double fc = 180.0 * std::pow (2.0, 6.2 * lfo);
            const float g = (float) std::tan (pi * std::min (fc, sr * 0.45) / sr), k = 0.6f;
            const float a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const float v3 = x - fic2, v1 = a1 * fic1 + a2 * v3, v2 = fic2 + a2 * fic1 + a3 * v3;
            fic1 = 2 * v1 - fic1; fic2 = 2 * v2 - fic2;
            wetR = v2; return v2;
        }
        case Fx::GrainDelay:
        {
            const int every = (int) (0.028 * sr), glen = (int) (0.085 * sr);
            if (grainClock++ % every == 0)
                for (auto& g : grains)
                    if (! g.on)
                    {
                        rng = rng * 1664525u + 1013904223u;
                        const double jitter = ((rng >> 8) % 1000) / 1000.0;
                        g.on = true; g.age = 0; g.len = glen;
                        g.rate = (prm.feedback > 0.6f && (rng & 1)) ? 2.0 : 1.0;
                        g.pos = hw - std::min (L * (1.0 + 0.15 * jitter), hs * 0.4) - (g.rate > 1 ? glen : 0);
                        const float pan = (float) jitter;
                        g.panL = std::sqrt (1 - pan); g.panR = std::sqrt (pan);
                        break;
                    }
            float yl = 0, yr = 0;
            for (auto& g : grains)
            {
                if (! g.on) continue;
                double rp = g.pos + g.age * g.rate; while (rp < 0) rp += hs; while (rp >= hs) rp -= hs;
                const int i0 = (int) rp; const float fr = (float) (rp - i0);
                const float smp = (hist[(size_t) i0] + fr * (hist[(size_t) ((i0 + 1) % hs)] - hist[(size_t) i0])) * hann ((double) g.age / g.len);
                yl += smp * g.panL; yr += smp * g.panR;
                if (++g.age >= g.len) g.on = false;
            }
            wetR = yr * 0.9f; return yl * 0.9f;
        }
        default: wetR = x; return x;
    }
}

// ---------------------------------------------------------------------------
void SoupEngine::render (float* outL, float* outR, int n)
{
    aInc = (float) (1.0 / (std::max (0.5f, prm.attackMs) * 0.001 * sr));
    dCoef = t60 (prm.decayMs * 0.001, sr);
    rCoef = t60 (prm.releaseMs * 0.001, sr);
    const float toneA = onePole (700, sr), smA = onePole (20, sr);
    const float levelT = std::pow (10.f, prm.levelDb / 20.f);
    const float upA = onePole (40, sr), downA = onePole (3, sr);
    const float damp = std::clamp (prm.damp, 0.f, 0.95f);
    const double beatInc = bpm / 60.0 * invSr;
    beatPos = ppq >= 0 ? ppq : internalBeats;
    const int hs = (int) hist.size();
    const double dBeats = divBeats[prm.division];
    float peak = 0;

    for (int i = 0; i < n; ++i)
    {
        // arpège (Bouillon d'arpèges) calé sur la division
        if (prm.arpOn && heldCount > 0)
        {
            const double stepNow = std::floor (beatPos / dBeats);
            if (stepNow != arpPhaseDiv)
            {
                arpPhaseDiv = stepNow;
                int seq[48]; int len = 0;
                int sorted[16]; std::copy (held.begin(), held.begin() + heldCount, sorted); std::sort (sorted, sorted + heldCount);
                for (int o = 0; o < prm.arpOct; ++o) for (int k = 0; k < heldCount && len < 48; ++k) seq[len++] = sorted[k] + 12 * o;
                arpIdx = prm.arpMode == 1 ? (arpIdx <= 0 ? len - 1 : arpIdx - 1) : (arpIdx + 1) % len;
                arpIdx = std::clamp (arpIdx, 0, len - 1);
                for (auto& v : voices) if (v.active && v.key == -1 && ! v.released) { v.released = true; v.stage = 3; }
                startVoice (seq[arpIdx], -1, 0.85f);
                fb.noteHits.fetch_add (1, std::memory_order_relaxed);
            }
            else if (beatPos / dBeats - stepNow > 0.6)
                for (auto& v : voices) if (v.active && v.key == -1 && ! v.released) { v.released = true; v.stage = 3; }
        }

        float x = 0;
        for (auto& v : voices) if (v.active) x += renderVoice (v);
        x *= 0.5f;

        // Tone (bascule grave/aigu)
        toneLow += toneA * (x - toneLow);
        const float tn = std::clamp (prm.tone, -1.f, 1.f);
        x = toneLow * (1 - 0.45f * tn) + (x - toneLow) * (1 + 0.8f * tn);

        hist[(size_t) hw] = x;

        // module d'effet du sachet, puis interrupteurs Reverse / Stutter
        float wr = x;
        float wl = processFx (prm.fx, x, wr, 0);
        if (prm.reverseOn || prm.stutterOn)
        {
            float m = 0.5f * (wl + wr);
            if (prm.reverseOn) m = reverseStage (m, 1);
            if (prm.stutterOn) m = stutterStage (m, 1);
            wl = wr = m;
        }
        hw = (hw + 1) % hs;

        mixSm += smA * (prm.mix - mixSm);
        const bool anyFx = prm.fx != Fx::None || prm.reverseOn || prm.stutterOn;
        const float mx = anyFx ? mixSm : 0.f;
        float L = x * (1 - mx) + wl * mx, R = x * (1 - mx) + wr * mx;
        const float wetAmt = std::abs (wl - x) * mx;
        wetFollower += (wetAmt > wetFollower ? upA : downA) * (wetAmt - wetFollower);

        // Space (réverbération, aigus amortis par Damp)
        spaceSm += smA * (prm.space - spaceSm);
        if (spaceSm > 0.001f)
        {
            const float in = (L + R) * 0.5f * 0.03f;
            float rl = 0, rr = 0;
            for (int c = 0; c < 8; ++c)
            {
                auto& cl = combL[(size_t) c]; auto& cr = combR[(size_t) c];
                float y = cl.b[(size_t) cl.i]; cl.store = y * (1 - damp) + cl.store * damp; cl.b[(size_t) cl.i] = in + cl.store * 0.86f; if (++cl.i >= (int) cl.b.size()) cl.i = 0; rl += y;
                y = cr.b[(size_t) cr.i]; cr.store = y * (1 - damp) + cr.store * damp; cr.b[(size_t) cr.i] = in + cr.store * 0.86f; if (++cr.i >= (int) cr.b.size()) cr.i = 0; rr += y;
            }
            for (int a = 0; a < 4; ++a)
            {
                auto& al = apL[(size_t) a]; auto& ar = apR[(size_t) a];
                float bo = al.b[(size_t) al.i]; al.b[(size_t) al.i] = rl + bo * 0.5f; rl = bo - rl; if (++al.i >= (int) al.b.size()) al.i = 0;
                bo = ar.b[(size_t) ar.i]; ar.b[(size_t) ar.i] = rr + bo * 0.5f; rr = bo - rr; if (++ar.i >= (int) ar.b.size()) ar.i = 0;
            }
            L = L * (1 - 0.35f * spaceSm) + rl * spaceSm * 1.6f;
            R = R * (1 - 0.35f * spaceSm) + rr * spaceSm * 1.6f;
        }

        follower += (std::abs (x) > follower ? upA : downA) * (std::abs (x) - follower);
        levelSm += smA * (levelT - levelSm);
        L *= levelSm; R *= levelSm;
        auto safe = [] (float o) { const float a = std::abs (o);
            if (a > 0.9f) o = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), o);
            return std::isfinite (o) ? o : 0.f; };
        L = safe (L); R = safe (R);
        outL[i] = L; if (outR) outR[i] = R;
        peak = std::max ({ peak, std::abs (L), std::abs (R) });
        beatPos += beatInc;
    }
    internalBeats = beatPos;
    if (ppq >= 0) ppq = -1;   // l'hôte redonne la position au bloc suivant

    fb.energy.store (std::min (1.f, follower * 3.f), std::memory_order_relaxed);
    fb.wet.store (std::min (1.f, wetFollower * 4.f), std::memory_order_relaxed);
    fb.beat.store ((float) (beatPos / dBeats - std::floor (beatPos / dBeats)), std::memory_order_relaxed);
    fb.voices.store (activeVoices(), std::memory_order_relaxed);
    fb.level.store (peak, std::memory_order_relaxed);
}
} // namespace soup
