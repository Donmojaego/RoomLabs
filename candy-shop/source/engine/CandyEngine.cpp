#include "CandyEngine.h"
#include <algorithm>
#include <cmath>

namespace cdy
{
namespace
{
constexpr double twoPi = 6.283185307179586;
constexpr double pi = 3.141592653589793;

inline float t60Coef (double seconds, double sr)
{
    return (float) std::exp (std::log (0.001) / (std::max (seconds, 0.0005) * sr));
}
inline float onePole (double hz, double sr) { return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr)); }
inline double polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

const int chordIv[8][4] = { { 0, -1, -1, -1 }, { 0, 4, 7, -1 }, { 0, 3, 7, -1 }, { 0, 4, 7, 11 },
                            { 0, 3, 7, 10 }, { 0, 5, 7, -1 }, { 0, 7, 12, -1 }, { 0, 4, 7, 14 } };
const double arpBeats[6] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };

// boîte à rythmes : réglages par kit
const float kitTune[5]  = { 1.f, 1.12f, 0.88f, 1.25f, 0.94f };
const float kitDecay[5] = { 1.f, 0.7f, 1.4f, 0.55f, 1.15f };
const float kitColor[5] = { 1.f, 1.3f, 0.75f, 1.6f, 1.1f };
// groupe d'affichage : 0 tam-tam, 1 tambour, 2 claves
const int drumGroup[12] = { 0, 2, 1, 1, 0, 2, 0, 2, 2, 1, 2, 1 };
} // namespace

void CandyEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1000.0 ? sampleRate : 48000.0;
    invSr = 1.0 / sr;
    chorusBuf.assign ((size_t) (sr * 0.05) + 4, 0.f);
    static const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static const int apT[4] = { 556, 441, 341, 225 };
    const double k = sr / 44100.0;
    for (int i = 0; i < 8; ++i)
    {
        combL[(size_t) i].b.assign ((size_t) (combT[i] * k) + 1, 0.f);
        combR[(size_t) i].b.assign ((size_t) ((combT[i] + 23) * k) + 1, 0.f);
    }
    for (int i = 0; i < 4; ++i)
    {
        apL[(size_t) i].b.assign ((size_t) (apT[i] * k) + 1, 0.f);
        apR[(size_t) i].b.assign ((size_t) ((apT[i] + 23) * k) + 1, 0.f);
    }
    reset();
}

void CandyEngine::reset()
{
    for (auto& v : voices) v = Voice {};
    for (auto& d : drums) d = Drum {};
    heldCount = 0; arpIndex = -1; arpDir = 1; arpCurrent = -1; arpPhase = 0.0; arpWasRunning = false;
    std::fill (chorusBuf.begin(), chorusBuf.end(), 0.f);
    for (auto* arr : { &combL, &combR }) for (auto& c : *arr) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; c.store = 0.f; }
    for (auto* arr : { &apL, &apR }) for (auto& a : *arr) { std::fill (a.b.begin(), a.b.end(), 0.f); a.i = 0; }
    follower = 0.f; toneLow = 0.f;
}

void CandyEngine::setParams (const Params& p)
{
    const bool arpTurnedOff = prm.arpOn && ! p.arpOn;
    prm = p;
    prm.unison = std::clamp (prm.unison, 1, 3);
    prm.chord = std::clamp (prm.chord, 0, 7);
    prm.arpRate = std::clamp (prm.arpRate, 0, 5);
    prm.arpOct = std::clamp (prm.arpOct, 1, 3);
    prm.kit = std::clamp (prm.kit, 0, 4);
    if (arpTurnedOff) releaseKey (-1);
}

float CandyEngine::noise()
{
    rng = rng * 1664525u + 1013904223u;
    return ((float) (rng >> 9) / 4194304.f) - 1.f;
}

int CandyEngine::activeVoices() const
{
    int n = 0; for (auto& v : voices) n += v.active ? 1 : 0; return n;
}

// ---------------------------------------------------------------------------
void CandyEngine::noteOn (int channel, int note, float vel)
{
    vel = std::clamp (vel, 0.f, 1.f);
    if (isDrum (channel, note))
    {
        Drum* d = nullptr;
        for (auto& x : drums) if (! x.active) { d = &x; break; }
        if (d == nullptr) { d = &drums[0]; for (auto& x : drums) if (x.t > d->t) d = &x; }
        *d = Drum {};
        d->active = true; d->slot = ((note % 12) + 12) % 12; d->vel = vel;
        fb.drumHits[drumGroup[d->slot]].fetch_add (1, std::memory_order_relaxed);
        return;
    }

    int w = 0;
    for (int i = 0; i < heldCount; ++i)
        if (held[(size_t) i] != note) { held[(size_t) w] = held[(size_t) i]; heldVel[(size_t) w] = heldVel[(size_t) i]; ++w; }
    heldCount = std::min (w, (int) held.size() - 1);
    held[(size_t) heldCount] = note; heldVel[(size_t) heldCount] = vel; ++heldCount;

    if (prm.arpOn)
    {
        if (! arpWasRunning) { arpPhase = 1.0; arpIndex = -1; arpDir = 1; arpWasRunning = true; }   // premier pas tout de suite
        return;
    }
    for (int k = 0; k < 4; ++k)
    {
        const int iv = chordIv[prm.chord][k];
        if (iv < 0) break;
        startVoice (note + iv, note, vel);
    }
    fb.noteHits.fetch_add (1, std::memory_order_relaxed);
}

void CandyEngine::noteOff (int channel, int note)
{
    if (isDrum (channel, note)) return;    // percussions : sons courts, sans relâchement
    int w = 0;
    for (int i = 0; i < heldCount; ++i)
        if (held[(size_t) i] != note) { held[(size_t) w] = held[(size_t) i]; heldVel[(size_t) w] = heldVel[(size_t) i]; ++w; }
    heldCount = w;
    releaseKey (note);
    if (heldCount == 0) { releaseKey (-1); arpWasRunning = false; }
}

void CandyEngine::allNotesOff()
{
    heldCount = 0; arpWasRunning = false;
    for (auto& v : voices) if (v.active) { v.released = true; v.stage = 3; }
}

void CandyEngine::releaseKey (int key)
{
    for (auto& v : voices)
        if (v.active && ! v.released && v.key == key) { v.released = true; v.stage = 3; }
}

void CandyEngine::startVoice (int note, int key, float vel)
{
    if (note < 0 || note > 127) return;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr)
    {
        float best = 2.f;
        for (auto& x : voices) if (x.released && x.env < best) { best = x.env; v = &x; }
    }
    if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }

    const bool wasActive = v->active;
    const float keepEnv = wasActive ? v->env : 0.f;
    *v = Voice {};
    v->active = true; v->note = note; v->key = key; v->vel = vel;
    v->hz = 440.0 * std::pow (2.0, (note + prm.tune - 69.0) / 12.0);
    v->env = keepEnv;           // voix volée : repart du niveau actuel, pas de clic
    v->age = ++ageCounter;
    v->ph[1] = 0.33; v->ph[2] = 0.71;
    v->partEnv[0] = v->partEnv[1] = v->partEnv[2] = 1.f;
}

// ---------------------------------------------------------------------------
void CandyEngine::advanceArp()
{
    // séquence : touches tenues (accord compris), étendue sur les octaves
    int base[64]; int nb = 0;
    for (int i = 0; i < heldCount && nb < 60; ++i)
        for (int k = 0; k < 4 && nb < 60; ++k)
        {
            const int iv = chordIv[prm.chord][k];
            if (iv < 0) break;
            base[nb++] = held[(size_t) i] + iv;
        }
    if (prm.arpMode != 4) std::sort (base, base + nb);
    arpSeqLen = 0;
    for (int o = 0; o < prm.arpOct; ++o)
        for (int i = 0; i < nb && arpSeqLen < 64; ++i) arpSeq[(size_t) arpSeqLen++] = base[i] + 12 * o;
    if (arpSeqLen == 0) return;

    switch (prm.arpMode)
    {
        case 1: arpIndex = arpIndex <= 0 ? arpSeqLen - 1 : arpIndex - 1; break;
        case 2:
            if (arpSeqLen == 1) arpIndex = 0;
            else
            {
                if (arpIndex < 0) { arpIndex = 0; arpDir = 1; break; }
                if (arpIndex + arpDir >= arpSeqLen || arpIndex + arpDir < 0) arpDir = -arpDir;
                arpIndex += arpDir;
            }
            break;
        case 3: rng = rng * 1664525u + 1013904223u; arpIndex = (int) ((rng >> 8) % (uint32_t) arpSeqLen); break;
        default: arpIndex = (arpIndex + 1) % arpSeqLen; break;
    }
    arpIndex = std::clamp (arpIndex, 0, arpSeqLen - 1);
    releaseKey (-1);
    const float vel = heldVel[(size_t) std::max (0, heldCount - 1)];
    startVoice (arpSeq[(size_t) arpIndex], -1, vel);
    arpCurrent = arpSeq[(size_t) arpIndex];
    fb.arpStep.fetch_add (1, std::memory_order_relaxed);
    fb.noteHits.fetch_add (1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
void CandyEngine::renderVoice (Voice& v, float& l, float& r)
{
    v.t += invSr;

    // enveloppe ADSR
    switch (v.stage)
    {
        case 0: v.env += aInc; if (v.env >= 1.f) { v.env = 1.f; v.stage = 1; } break;
        case 1: v.env = prm.sustain + (v.env - prm.sustain) * dCoef; break;
        default: v.env *= rCoef; break;
    }
    if (v.env < 1.0e-5f && (v.stage == 3 || (v.stage == 1 && prm.sustain <= 0.0005f && v.t > 0.02)))
    {
        v.active = false; v.env = 0.f; return;
    }

    const double drop = prm.pitchDrop > 0.f ? prm.pitchDrop * std::exp (-v.t / 0.03) : 0.0;
    const double hz = v.hz * (drop > 0.0 ? std::pow (2.0, drop / 12.0) : 1.0);
    float s = 0.f;

    switch (prm.algo)
    {
        case Algo::Mallet:
        {
            static const double ratio[3] = { 1.0, 3.93, 9.24 };
            static const float amp[3] = { 1.f, 0.38f, 0.14f };
            const float bright = 0.6f + 0.4f * prm.tone;
            for (int i = 0; i < 3; ++i)
            {
                const double f = hz * ratio[i];
                if (f >= sr * 0.45) continue;
                v.partPh[i] += f * invSr; if (v.partPh[i] >= 1.0) v.partPh[i] -= 1.0;
                v.partEnv[i] *= i == 0 ? 1.f : (i == 1 ? modDecCoef : modDecCoef * modDecCoef);
                s += amp[i] * (i == 0 ? 1.f : bright) * v.partEnv[i] * (float) std::sin (v.partPh[i] * twoPi);
            }
            if (v.t < 0.006) s += 0.25f * noise() * (float) std::exp (-v.t / 0.0015);   // coup de mailloche
            s *= 0.75f;
            break;
        }
        case Algo::Fm:
        {
            v.partEnv[0] *= modDecCoef;
            v.modPh += hz * prm.fmRatio * invSr; if (v.modPh >= 1.0) v.modPh -= 1.0;
            const double idx = prm.fmAmount * 7.0 * v.partEnv[0];
            v.ph[0] += hz * invSr; if (v.ph[0] >= 1.0) v.ph[0] -= 1.0;
            s = (float) std::sin (v.ph[0] * twoPi + idx * std::sin (v.modPh * twoPi));
            break;
        }
        case Algo::Synth:
        default:
        {
            const int U = prm.unison;
            for (int u = 0; u < U; ++u)
            {
                const double cents = (u - (U - 1) * 0.5) * prm.detune;
                const double f = hz * std::pow (2.0, cents / 1200.0);
                const double dt = f * invSr;
                double& p = v.ph[u];
                p += dt; if (p >= 1.0) p -= 1.0;
                double o;
                switch (prm.wave)
                {
                    case 0: o = std::sin (p * twoPi); break;
                    case 1: o = 1.0 - 4.0 * std::abs (p - 0.5); break;
                    case 3: { double t2 = p + 0.5; if (t2 >= 1.0) t2 -= 1.0;
                              o = (p < 0.5 ? 1.0 : -1.0) + polyBlep (p, dt) - polyBlep (t2, dt); break; }
                    default: o = 2.0 * p - 1.0 - polyBlep (p, dt); break;
                }
                s += (float) o;
            }
            s /= (float) std::sqrt ((double) U);
            s *= 0.6f;
            break;
        }
    }
    if (prm.noise > 0.f) s += prm.noise * 0.5f * noise();

    // filtre passe-bas (SVF TPT) avec enveloppe — pas pour la mailloche
    if (prm.algo != Algo::Mallet)
    {
        v.fenvLevel *= fdCoef;
        if ((v.filterTick++ & 7) == 0)      // coefficients recalculés tous les 8 échantillons
        {
            const double fc = std::min ((double) prm.cutoff * std::pow (2.0, prm.fenv * 6.0 * v.fenvLevel), sr * 0.45);
            const float g = (float) std::tan (pi * fc / sr);
            const float k = 2.f - 1.9f * std::clamp (prm.reso, 0.f, 1.f);
            v.a1 = 1.f / (1.f + g * (g + k)); v.a2 = g * v.a1; v.a3 = g * v.a2;
        }
        const float a1 = v.a1, a2 = v.a2, a3 = v.a3;
        const float v3 = s - v.ic2, v1 = a1 * v.ic1 + a2 * v3, v2 = v.ic2 + a2 * v.ic1 + a3 * v3;
        v.ic1 = 2.f * v1 - v.ic1; v.ic2 = 2.f * v2 - v.ic2;
        s = v2;
    }

    const float velGain = 1.f - prm.velSens + prm.velSens * v.vel;
    const float out = s * v.env * velGain;
    l += out; r += out;
}

float CandyEngine::renderDrum (Drum& d)
{
    const int k = prm.kit;
    const float tu = kitTune[k], dc = kitDecay[k], col = kitColor[k];
    d.t += invSr;
    const double t = d.t;
    float s = 0.f, len = 0.5f;
    auto tone = [&] (double f0, double dropMul, double dropT, double dec) {
        const double f = f0 * tu * (1.0 + (dropMul - 1.0) * std::exp (-t / dropT));
        d.ph += f * invSr; if (d.ph >= 1.0) d.ph -= 1.0;
        return (float) (std::sin (d.ph * twoPi) * std::exp (-t / (dec * dc)));
    };
    auto band = [&] (double center, double dec) {
        const float n = noise();
        const float a = onePole (center * col * 0.6, sr), b = onePole (center * col * 1.6, sr);
        d.hp += a * (n - d.hp);           // lp1
        const float hpOut = n - d.hp;     // passe-haut
        d.bp1 += b * (hpOut - d.bp1);     // passe-bas
        return d.bp1 * 1.8f * (float) std::exp (-t / (dec * dc));
    };
    switch (d.slot)
    {
        case 0: s = tone (95, 1.6, 0.02, 0.32) * 1.1f + band (900, 0.006) * 0.4f; len = 2.4f; break;         // tam-tam grave
        case 1: s = tone (2400, 1.0, 1, 0.022) + 0.3f * (float) (std::sin (d.ph * twoPi * 2.2) * std::exp (-t / 0.01)); len = 0.15f; break; // claves
        case 2: s = tone (230, 1.3, 0.01, 0.08) * 0.7f + band (2200, 0.13) * 0.8f; len = 0.7f; break;      // tambour
        case 3: { const double tt = t < 0.03 ? std::fmod (t, 0.01) : t - 0.02;                             // claquement
                  s = band (1100, 0.12) * (float) (std::exp (-tt / 0.006) * 0.8 + (t > 0.02 ? 0.5 * std::exp (-(t - 0.02) / (0.12 * dc)) : 0.0)) * 2.f; len = 0.6f; break; }
        case 4: s = tone (140, 1.5, 0.018, 0.26) * 1.05f + band (1100, 0.005) * 0.3f; len = 1.9f; break;    // tam-tam médium
        case 5: s = band (5200, 0.004) * 1.4f + tone (3200, 1.0, 1, 0.008) * 0.4f; len = 0.08f; break;      // tic d'horloge
        case 6: s = tone (210, 1.4, 0.015, 0.2) + band (1400, 0.005) * 0.3f; len = 1.5f; break;            // tam-tam aigu
        case 7: s = tone (1700, 1.0, 1, 0.012) + band (3000, 0.003) * 0.6f; len = 0.1f; break;             // tac d'horloge
        case 8: { d.ph2 += 540.0 * tu * 1.41 * invSr; if (d.ph2 >= 1.0) d.ph2 -= 1.0;                      // machine métal
                  const double env = std::exp (-t / (0.18 * dc));
                  d.ph += 540.0 * tu * invSr; if (d.ph >= 1.0) d.ph -= 1.0;
                  s = (float) (std::sin (d.ph * twoPi + 4.0 * env * std::sin (d.ph2 * twoPi)) * env) * 0.7f; len = 1.0f; break; }
        case 9: { const float att = (float) std::min (1.0, t / 0.005);                                     // shaker
                  s = band (7000, 0.06) * att; len = 0.4f; break; }
        case 10: { const double f = (120.0 + 1700.0 * std::exp (-t / 0.04)) * tu;                          // ressort / zap
                   d.ph += f * invSr; if (d.ph >= 1.0) d.ph -= 1.0;
                   s = (float) (std::sin (d.ph * twoPi) * std::exp (-t / (0.12 * dc))) * 0.8f; len = 0.8f; break; }
        default: s = tone (820, 1.2, 0.004, 0.03) * 0.8f + band (2600, 0.025) * 0.7f; len = 0.25f; break;  // petit tambour sec
    }
    if (t > len * dc + 0.05) d.active = false;
    return s * (0.35f + 0.65f * d.vel);
}

// ---------------------------------------------------------------------------
void CandyEngine::render (float* outL, float* outR, int n)
{
    aInc = (float) (1.0 / (std::max (0.5f, prm.attackMs) * 0.001 * sr));
    dCoef = t60Coef (prm.decayMs * 0.001, sr);
    rCoef = t60Coef (prm.releaseMs * 0.001, sr);
    fdCoef = t60Coef (prm.fdecayMs * 0.001, sr);
    modDecCoef = t60Coef (prm.fmDecay, sr);
    const float toneA = onePole (700.0, sr);
    const float levelT = std::pow (10.f, prm.levelDb / 20.f);
    const float drumT = std::pow (10.f, prm.drumLevelDb / 20.f);
    const float smA = onePole (25.0, sr);
    const double stepSamples = std::max (8.0, 60.0 / tempo * arpBeats[prm.arpRate] * sr);
    const float follUp = onePole (40.0, sr), follDown = onePole (3.0, sr);
    float peak = 0.f;

    for (int i = 0; i < n; ++i)
    {
        // arpège calé au tempo
        if (prm.arpOn && heldCount > 0)
        {
            const double before = arpPhase;
            arpPhase += 1.0 / stepSamples;
            if (before < prm.arpGate && arpPhase >= prm.arpGate) releaseKey (-1);
            if (arpPhase >= 1.0) { arpPhase -= std::floor (arpPhase); advanceArp(); }
        }

        float l = 0.f, r = 0.f;
        for (auto& v : voices) if (v.active) renderVoice (v, l, r);
        float x = 0.5f * (l + r) * 0.45f;

        // Tone : bascule grave/aigu
        toneLow += toneA * (x - toneLow);
        const float tn = std::clamp (prm.tone, -1.f, 1.f);
        x = toneLow * (1.f - 0.45f * tn) + (x - toneLow) * (1.f + 0.8f * tn);

        // Chorus stéréo
        float cl = x, cr = x;
        if (prm.chorus > 0.001f)
        {
            const int sz = (int) chorusBuf.size();
            chorusBuf[(size_t) chorusW] = x;
            chorusLfo += 0.6 * invSr; if (chorusLfo >= 1.0) chorusLfo -= 1.0;
            auto tap = [&] (double ms) {
                double rp = chorusW - ms * 0.001 * sr; while (rp < 0) rp += sz;
                const int i0 = (int) rp, i1 = (i0 + 1) % sz; const float fr = (float) (rp - i0);
                return chorusBuf[(size_t) i0] + fr * (chorusBuf[(size_t) i1] - chorusBuf[(size_t) i0]); };
            const float dl = tap (9.0 + 3.0 * std::sin (chorusLfo * twoPi));
            const float dr = tap (11.0 + 3.0 * std::cos (chorusLfo * twoPi));
            chorusW = (chorusW + 1) % sz;
            cl = x + prm.chorus * (dl - 0.3f * x);
            cr = x + prm.chorus * (dr - 0.3f * x);
        }

        // énergie pour l'interface (instrument seul)
        const float ax = std::abs (x);
        follower += (ax > follower ? follUp : follDown) * (ax - follower);

        // percussions
        float dsum = 0.f;
        for (auto& d : drums) if (d.active) dsum += renderDrum (d);
        dsum *= drumT * 0.6f;

        // Space : réverbération (type Freeverb)
        spaceSm += smA * (prm.space - spaceSm);
        float wl = 0.f, wr = 0.f;
        if (spaceSm > 0.001f)
        {
            const float in = (cl + cr) * 0.5f * 0.03f + dsum * 0.006f;
            for (int c = 0; c < 8; ++c)
            {
                for (auto* cb : { &combL[(size_t) c], &combR[(size_t) c] })
                {
                    float& o = cb == &combL[(size_t) c] ? wl : wr;
                    const float y = cb->b[(size_t) cb->i];
                    cb->store = y * 0.8f + cb->store * 0.2f;
                    cb->b[(size_t) cb->i] = in + cb->store * 0.86f;
                    if (++cb->i >= (int) cb->b.size()) cb->i = 0;
                    o += y;
                }
            }
            for (int a = 0; a < 4; ++a)
                for (auto* ap : { &apL[(size_t) a], &apR[(size_t) a] })
                {
                    float& o = ap == &apL[(size_t) a] ? wl : wr;
                    const float bo = ap->b[(size_t) ap->i];
                    ap->b[(size_t) ap->i] = o + bo * 0.5f;
                    o = bo - o;
                    if (++ap->i >= (int) ap->b.size()) ap->i = 0;
                }
        }
        const float dry = 1.f - 0.35f * spaceSm;
        float L = cl * dry + wl * spaceSm * 1.6f + dsum;
        float R = cr * dry + wr * spaceSm * 1.6f + dsum;

        levelSm += smA * (levelT - levelSm);
        L *= levelSm; R *= levelSm;
        auto safe = [] (float o) {
            const float a = std::abs (o);
            if (a > 0.9f) o = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), o);
            return std::isfinite (o) ? o : 0.f; };
        L = safe (L); R = safe (R);
        outL[i] = L; if (outR) outR[i] = R;
        peak = std::max (peak, std::max (std::abs (L), std::abs (R)));
    }

    fb.energy.store (std::min (1.f, follower * 3.f), std::memory_order_relaxed);
    fb.level.store (peak, std::memory_order_relaxed);
    fb.voices.store (activeVoices(), std::memory_order_relaxed);
}
} // namespace cdy
