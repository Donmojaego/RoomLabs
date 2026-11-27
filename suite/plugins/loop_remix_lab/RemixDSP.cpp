#include "RemixDSP.h"
#include <algorithm>
#include <cmath>

namespace rl::rx
{
namespace { constexpr double PI = 3.14159265358979323846; }

void Engine::prepare (double s)
{
    sr = s; size = (int) (sr * 8) + 8;
    for (auto& b : buf) b.assign ((size_t) size, 0.f);
    reset();
}

void Engine::reset()
{
    for (auto& b : buf) std::fill (b.begin(), b.end(), 0.f);
    abs = 0; curIdx = -1; fade = 0; gS = 1; crushS = sweepS = ringS = panS = 0;
    for (int c = 0; c < 2; ++c) { ic1[c] = ic2[c] = hold[c] = 0; shTime[c] = -1; shVal[c] = 0; }
    holdN = 0; ringPh = 0; seed = 12345; ppq = 0;
}

void Engine::setTransport (double b, double p, bool playing)
{
    bpm = std::clamp (b > 0 ? b : 120.0, 30.0, 300.0); hostPlay = playing;
    // on ne se recale sur l'hôte que s'il y a un vrai saut (évite les arrondis d'un bloc à l'autre)
    if (playing && std::abs (p - ppq) > 2.0 * bpm / 60.0 / sr) ppq = p;
}

float Engine::read (int ch, double pos) const
{
    // pos : position absolue (échantillons) ; jamais dans le futur, jamais plus vieux que le tampon
    pos = std::clamp (pos, (double) (abs - size + 4), (double) abs);
    if (pos < 0) return 0.f;
    const int64_t i0 = (int64_t) std::floor (pos); const float fr = (float) (pos - (double) i0);
    const int64_t i1 = std::min (i0 + 1, abs);
    const auto& b = buf[ch];
    return b[(size_t) (i0 % size)] * (1 - fr) + b[(size_t) (i1 % size)] * fr;
}

void Engine::render (int fx, int64_t start, double t, double L, float& l, float& r, float& g) const
{
    const double s = (double) start; double pos = s + t; g = 1;
    switch (fx)
    {
        case Repeat:   pos = s + std::fmod (t, L * 0.25); break;
        case Roll:     pos = s + std::fmod (t, L * 0.125); break;
        case Reverse:  pos = s - 1 - std::fmod (t, L); break;
        case Half:     pos = s + t * 0.5; break;
        case TapeStop: { const double u = std::min (t, L); pos = s + u - u * u / (2 * L); break; }
        case Scratch:  pos = s + t - (L / 6) * 0.5 * (1 - std::cos (2 * PI * t / (L * 0.5))); break;
        case Gate:     g = std::fmod (t, L * 0.5) < L * 0.25 ? 1.f : 0.f; break;
        case Mute:     g = 0; break;
        case PitchUp:  pos = s - L + std::fmod (2 * t, L); break;
        case Drift:    pos = s + t - (L / 12) * 0.5 * (1 - std::cos (2 * PI * t / L)); break;
        default: break;
    }
    l = read (0, pos); r = read (1, pos);
    if (fx == Echo) { const double e = s + t - 0.75 * L; l += 0.55f * read (0, e); r += 0.55f * read (1, e); l *= 0.8f; r *= 0.8f; }
}

double Engine::lfoValue (int k, double q)
{
    const auto& o = prm.lfo[k]; const double rate = kRates[std::clamp (o.rate, 0, 6)];
    const double ph = q / rate - std::floor (q / rate), cyc = std::floor (q / rate);
    switch (o.shape)
    {
        case 0: return std::sin (2 * PI * ph);
        case 1: return ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph;
        case 2: return 1 - 2 * ph;
        case 3: return ph < 0.5 ? 1 : -1;
        default:
            if (cyc != shTime[k]) { shTime[k] = cyc; seed = seed * 1664525u + 1013904223u; shVal[k] = (float) ((seed >> 8) / 8388608.0 - 1.0); }
            return shVal[k];
    }
}

void Engine::process (float* const* io, int numCh, int n)
{
    const double L = 60.0 / bpm / 4.0 * sr;            // durée d'un pas (double-croche)
    const double dq = bpm / 60.0 / sr;                  // noires par échantillon
    const float sm = (float) (1 - std::exp (-1.0 / (0.002 * sr))), fadeInc = (float) (1.0 / (0.003 * sr));
    const float outG = std::pow (10.f, prm.outDb / 20.f);
    for (int i = 0; i < n; ++i)
    {
        const float dl = io[0][i], dr = numCh > 1 ? io[1][i] : dl;
        buf[0][(size_t) (abs % size)] = dl; buf[1][(size_t) (abs % size)] = dr;
        const double q = ppq + i * dq;
        const double qs = q * 4.0 + 1e-7; int idx = (int) std::floor (qs) % 16; if (idx < 0) idx += 16;
        if (idx != curIdx)
        {
            prevFx = curIdx < 0 ? Thru : curFx; prevStart = stepStart; fade = curIdx < 0 ? 0.f : 1.f;
            curIdx = idx; curFx = std::clamp (prm.steps[(size_t) idx], 0, NumFx - 1);
            // le pas démarre à la vraie frontière (ppq), même si le bloc commence au milieu
            const double into = (qs - std::floor (qs)) * L;
            stepStart = abs - (int64_t) std::llround (std::min (into, L - 1));
        }
        float l, r, g; render (curFx, stepStart, (double) (abs - stepStart), L, l, r, g);
        if (fade > 0)
        {
            float pl, pr, pg; render (prevFx, prevStart, (double) (abs - prevStart), L, pl, pr, pg);
            l = l * (1 - fade) + pl * fade; r = r * (1 - fade) + pr * fade; g = g * (1 - fade) + pg * fade;
            fade = std::max (0.f, fade - fadeInc);
        }
        // couleurs (lissées) selon l'effet du pas
        const double t = (double) (abs - stepStart);
        gS += sm * (g - gS);
        crushS += sm * ((curFx == Crush ? 1.f : 0.f) - crushS);
        sweepS += sm * ((curFx == Sweep ? (float) std::min (1.0, t / L) : 0.f) - sweepS);
        ringS += sm * ((curFx == Ring ? 1.f : 0.f) - ringS);
        panS += sm * ((curFx == Pan ? (std::fmod (t, L * 0.5) < L * 0.25 ? -1.f : 1.f) : 0.f) - panS);
        // LFO
        float mF = 0, mV = 0, mP = 0, mC = 0, mR = 0, mQ = 0;
        float lv[2];
        for (int k = 0; k < 2; ++k)
        {
            lv[k] = (float) lfoValue (k, q); const float v = lv[k] * prm.lfo[k].depth;
            switch (prm.lfo[k].dest) { case 0: mF += v; break; case 1: mV += v; break; case 2: mP += v; break; case 3: mC += v; break; case 4: mR += v; break; default: mQ += v; }
        }
        // crush (bits + échantillonnage)
        const float cr = std::clamp (crushS + std::abs (mC), 0.f, 1.f);
        if (cr > 0.001f)
        {
            const int hn = 1 + (int) (cr * 11); const float lev = std::pow (2.f, 12.f - cr * 9.f);
            if (++holdN >= hn) { holdN = 0; hold[0] = std::round (l * lev) / lev; hold[1] = std::round (r * lev) / lev; }
            l += cr * (hold[0] - l); r += cr * (hold[1] - r);
        }
        // ring
        const float rg = std::clamp (ringS + std::abs (mR), 0.f, 1.f);
        ringPh += 180.0 / sr; if (ringPh >= 1) ringPh -= 1;
        if (rg > 0.001f) { const float m = (float) std::sin (2 * PI * ringPh); l *= 1 - rg + rg * m; r *= 1 - rg + rg * m; }
        // filtre passe-bas (SVF TPT)
        const bool filt = prm.cutoff < 19500.f || sweepS > 1e-4f || std::abs (mF) > 1e-4f || std::abs (mQ) > 1e-4f;
        const double fc = std::clamp ((double) prm.cutoff * std::pow (2.0, -6.0 * sweepS + 3.0 * mF), 30.0, sr * 0.45);
        const double gg = std::tan (PI * fc / sr); const double k = 2.0 - 1.9 * std::clamp (prm.reso + 0.5f * mQ, 0.f, 0.97f);
        const double a1 = 1 / (1 + gg * (gg + k)), a2 = gg * a1, a3 = gg * a2;
        float* ch[2] = { &l, &r };
        for (int c = 0; c < 2 && filt; ++c)
        {
            const double v3 = *ch[c] - ic2[c], v1 = a1 * ic1[c] + a2 * v3, v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
            ic1[c] = (float) (2 * v1 - ic1[c]); ic2[c] = (float) (2 * v2 - ic2[c]); *ch[c] = (float) v2;
        }
        // pan + volume
        const float pn = std::clamp (panS + mP, -1.f, 1.f);
        l *= 1 - std::max (0.f, pn); r *= 1 + std::min (0.f, pn);
        const float vol = gS * std::clamp (1.f - 0.5f * (1.f - (mV >= 0 ? 1.f - mV : 1.f + mV)) * 2.f, 0.f, 1.f);
        l *= vol; r *= vol;
        float ol = (dl * (1 - prm.mix) + l * prm.mix) * outG, orr = (dr * (1 - prm.mix) + r * prm.mix) * outG;
        if (! std::isfinite (ol) || ! std::isfinite (orr)) { ol = orr = 0; ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0; }
        ol = std::clamp (ol, -2.f, 2.f); orr = std::clamp (orr, -2.f, 2.f);
        io[0][i] = ol; if (numCh > 1) io[1][i] = orr;
        envAcc = std::max (envAcc, std::abs (ol)); if (++envN >= (int) (L / 4)) { meter.wave[(size_t) wavePos].store (envAcc); wavePos = (wavePos + 1) % 64; envAcc = 0; envN = 0; }
        meter.lfo1.store (lv[0]); meter.lfo2.store (lv[1]);
        ++abs;
    }
    ppq += n * dq; (void) hostPlay;
    meter.step.store (curIdx < 0 ? 0 : curIdx);
}
} // namespace rl::rx
