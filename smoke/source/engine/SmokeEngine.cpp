#include "SmokeEngine.h"
#include <algorithm>
#include <cmath>

namespace smk
{
namespace
{
constexpr double twoPi = 6.283185307179586, pi = 3.141592653589793;
inline float t60 (double s, double sr) { return (float) std::exp (std::log (0.001) / (std::max (s, 0.0005) * sr)); }
inline float onePole (double hz, double sr) { return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr)); }
// sinus par table (2048 points + interpolation linéaire, erreur < 1e-6) : phase en tours [0, 1)
struct SinTable { float t[2049]; SinTable() { for (int i = 0; i <= 2048; ++i) t[i] = (float) std::sin (twoPi * i / 2048.0); } };
inline float fastSin (double ph)
{
    static const SinTable tab;
    const double x = ph * 2048.0; const int i = (int) x & 2047; const float f = (float) (x - std::floor (x));
    return tab.t[i] + f * (tab.t[i + 1] - tab.t[i]);
}
inline double polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}
const int chordIv[6][5] = { { 0, -1, -1, -1, -1 }, { 0, 3, 7, 10, 14 }, { 0, 4, 7, 11, 14 }, { 0, 3, 10, 14, 17 }, { 0, 4, 10, 14, -1 }, { 0, 4, 7, 11, -1 } };
} // namespace

void SmokeEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1000 ? sampleRate : 48000; invSr = 1.0 / sr;
    chL.assign ((size_t) (sr * 0.05) + 4, 0.f); chR.assign (chL.size(), 0.f);
    pre.assign ((size_t) (sr * 0.12) + 4, 0.f);
    static const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }, apT[4] = { 556, 441, 341, 225 };
    const double k = sr / 44100.0 * 1.15;   // salle un peu plus grande
    for (int i = 0; i < 8; ++i) { combL[(size_t) i].b.assign ((size_t) (combT[i] * k) + 1, 0.f); combR[(size_t) i].b.assign ((size_t) ((combT[i] + 23) * k) + 1, 0.f); }
    for (int i = 0; i < 4; ++i) { apL[(size_t) i].b.assign ((size_t) (apT[i] * k) + 1, 0.f); apR[(size_t) i].b.assign ((size_t) ((apT[i] + 23) * k) + 1, 0.f); }
    reset();
}

void SmokeEngine::reset()
{
    for (auto& v : voices) v = Voice {};
    std::fill (chL.begin(), chL.end(), 0.f); std::fill (chR.begin(), chR.end(), 0.f); chW = 0;
    std::fill (pre.begin(), pre.end(), 0.f); preW = 0;
    for (auto* a : { &combL, &combR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; c.store = 0; }
    for (auto* a : { &apL, &apR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; }
    heldCount = 0; follower = wetFollow = 0; toneL1 = toneL2 = toneR1 = toneR2 = 0;
}

void SmokeEngine::setParams (const Params& p) { prm = p; prm.chord = std::clamp (prm.chord, 0, 5); }
int SmokeEngine::activeVoices() const { int n = 0; for (auto& v : voices) n += v.active; return n; }

// ---------------------------------------------------------------------------
void SmokeEngine::noteOn (int note, float vel)
{
    vel = std::clamp (vel, 0.f, 1.f);
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = std::min (w, (int) held.size() - 1);
    held[(size_t) heldCount++] = note;
    fb.noteHits.fetch_add (1, std::memory_order_relaxed);
    fb.lastVel.store (vel, std::memory_order_relaxed);

    if (isMono (prm.model))
    {
        // basse mono : glisse vers la nouvelle note si une autre est tenue (legato)
        for (auto& v : voices)
            if (v.active && ! v.released && v.key >= 0)
            {
                v.glideFrom = v.hz; v.glideTo = 440.0 * std::pow (2.0, (note + prm.tune - 69.0) / 12.0); v.glidePos = 0;
                v.note = note; v.key = note; return;
            }
        startVoice (note, note, vel);
        return;
    }
    for (int k = 0; k < 5; ++k)
    {
        const int iv = chordIv[prm.chord][k];
        if (iv < 0) break;
        startVoice (note + iv, note, vel * (k ? 0.8f : 1.f));
    }
}

void SmokeEngine::noteOff (int note)
{
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = w;
    if (isMono (prm.model))
    {
        for (auto& v : voices)
            if (v.active && ! v.released && v.key == note)
            {
                if (heldCount > 0)
                {
                    const int back = held[(size_t) heldCount - 1];
                    v.glideFrom = v.hz; v.glideTo = 440.0 * std::pow (2.0, (back + prm.tune - 69.0) / 12.0); v.glidePos = 0;
                    v.note = back; v.key = back;
                }
                else { v.released = true; v.stage = 3; }
            }
        return;
    }
    for (auto& v : voices) if (v.active && ! v.released && v.key == note) { v.released = true; v.stage = 3; }
}

void SmokeEngine::allNotesOff() { heldCount = 0; for (auto& v : voices) if (v.active) { v.released = true; v.stage = 3; } }

void SmokeEngine::startVoice (int note, int key, float vel)
{
    if (note < 0 || note > 127) return;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (! v) { float best = 2; for (auto& x : voices) if (x.released && x.env < best) { best = x.env; v = &x; } }
    if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    const float keep = v->active ? v->env : 0.f;
    *v = Voice {};
    v->active = true; v->note = note; v->key = key; v->vel = vel; v->env = keep; v->age = ++ageCounter;
    v->hz = 440.0 * std::pow (2.0, (note + prm.tune - 69.0) / 12.0);
    v->glideFrom = v->glideTo = v->hz;
    rng = rng * 1664525u + 1013904223u;
    v->pan = (((rng >> 8) & 1023) / 1023.f - 0.5f) * 0.6f;          // place stéréo propre à chaque note
    v->driftPh[0] = ((rng >> 4) & 255) / 255.0; v->driftPh[1] = ((rng >> 12) & 255) / 255.0; v->panPh = ((rng >> 20) & 255) / 255.0;
    for (auto& p : v->partEnv) p = 1.f;
    {
        // amplitudes des partiels du piano feutré : calculées une fois par note (plus doux quand on joue doucement)
        const double br = std::clamp ((double) prm.bright, 0.0, 1.0);
        for (int n = 1; n <= 8; ++n) v->partAmp[n - 1] = (float) (1.0 / std::pow ((double) n, 1.3 + (1.0 - vel) * 1.0 + (1.0 - br) * 0.6));
    }
    v->ph[1] = 0.33; v->ph[2] = 0.71;
}

// ---------------------------------------------------------------------------
void SmokeEngine::renderVoice (Voice& v, float& l, float& r)
{
    v.t += invSr;
    switch (v.stage)
    {
        case 0: v.env += aInc; if (v.env >= 1) { v.env = 1; v.stage = 1; } break;
        case 1: v.env = prm.sustain + (v.env - prm.sustain) * dCoef; break;
        default: v.env *= rCoef; break;
    }
    if (v.env < 1e-5f && v.stage == 3) { v.active = false; return; }

    // glide (basses)
    if (v.glidePos < 1.0)
    {
        v.glidePos = std::min (1.0, v.glidePos + 1.0 / (std::max (1.f, prm.glideMs) * 0.001 * sr));
        v.hz = v.glideFrom * std::pow (v.glideTo / v.glideFrom, v.glidePos);
    }
    // dérive de hauteur lente (Mod) : deux sinus lents propres à chaque note
    v.driftPh[0] += 0.13 * invSr; v.driftPh[1] += 0.31 * invSr;
    const double drift = prm.mod * 14.0 * (std::sin (twoPi * v.driftPh[0]) * 0.7 + std::sin (twoPi * v.driftPh[1]) * 0.3);
    double hz = v.hz * (1.0 + drift * 0.00057762265);   // 2^(c/1200) ≈ 1 + c·ln2/1200 (|c| ≤ 14 cents)

    const float br = std::clamp (prm.bright, 0.f, 1.f);
    float s = 0, cutoff = 6000, reso = 0.1f;
    switch (prm.model)
    {
        case Model::Rhodes:
        {
            v.modEnv *= kRhodesMod; v.tineEnv *= kRhodesTine;
            v.mph += hz * invSr; if (v.mph >= 1) v.mph -= 1;
            v.ph[0] += hz * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            v.ph[1] += hz * 14.0 * invSr; if (v.ph[1] >= 1) v.ph[1] -= 1;
            const double idx = (0.6 + 2.4 * br * v.vel) * (0.25 + 0.75 * v.modEnv);
            s = (float) std::sin (twoPi * v.ph[0] + idx * std::sin (twoPi * v.mph)) * 0.75f
              + 0.18f * v.vel * v.tineEnv * (float) std::sin (twoPi * v.ph[1]);
            cutoff = 2500 + 9000 * br; break;
        }
        case Model::Felt:
        {
            // piano feutré : 8 partiels légèrement inharmoniques, plus doux quand on joue doucement
            static const std::array<double, 8> ratio = [] { std::array<double, 8> a {}; for (int n = 1; n <= 8; ++n) a[(size_t) n - 1] = n * std::sqrt (1.0 + 0.00035 * n * n); return a; }();
            for (int n = 1; n <= 8; ++n)
            {
                const double f = hz * ratio[(size_t) n - 1];
                if (f >= sr * 0.45) break;
                double& p = v.ph[n - 1]; p += f * invSr; if (p >= 1) p -= 1;
                v.partEnv[n - 1] *= feltCoef[(size_t) n - 1];
                s += v.partAmp[n - 1] * v.partEnv[n - 1] * fastSin (p);
            }
            s *= 1.0f; cutoff = 1500 + 7000 * br * (0.4f + 0.6f * v.vel);
            if (v.t < 0.012) s *= (float) (v.t / 0.012);   // marteau feutré : pas de clic
            break;
        }
        case Model::Pad:
        case Model::Riser:
        {
            const bool riser = prm.model == Model::Riser;
            const double rise = riser ? std::min (1.0, v.t / std::max (0.2f, prm.riseS)) : 0.0;
            const double rh = hz * (riser ? std::pow (2.0, (rise * rise) * 7.0 / 12.0) : 1.0);   // la hauteur monte d'une quinte
            for (int u = 0; u < 3; ++u)
            {
                static constexpr double det[3] = { 0.99481489, 1.0, 1.00521214 };   // -9, 0, +9 cents
                const double f = rh * det[u], dt = f * invSr;
                v.ph[u] += dt; if (v.ph[u] >= 1) v.ph[u] -= 1;
                s += (float) (2.0 * v.ph[u] - 1.0 - polyBlep (v.ph[u], dt)) * 0.35f;
            }
            v.panPh += 0.07 * invSr;
            const float sweep = (float) (0.5 + 0.5 * std::sin (twoPi * v.panPh * 1.7));
            cutoff = (float) ((riser ? 300.0 + 9000.0 * rise * rise : 700.0 + 2200.0 * sweep) * (0.5 + br));
            reso = riser ? 0.35f : 0.15f; break;
        }
        case Model::Sub:
        {
            v.ph[0] += hz * invSr; if (v.ph[0] >= 1) v.ph[0] -= 1;
            s = (float) std::sin (twoPi * v.ph[0]) + br * 0.25f * (float) std::sin (2 * twoPi * v.ph[0]) + br * 0.12f * (float) std::sin (3 * twoPi * v.ph[0]);
            s *= 1.5f; cutoff = 1200; break;
        }
        case Model::HouseBass:
        case Model::Count:
        default:
        {
            const double dt = hz * invSr;
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            double t2 = v.ph[0] + 0.5; if (t2 >= 1) t2 -= 1;
            const double sq = (v.ph[0] < 0.5 ? 1.0 : -1.0) + polyBlep (v.ph[0], dt) - polyBlep (t2, dt);
            const double sw = 2.0 * v.ph[0] - 1.0 - polyBlep (v.ph[0], dt);
            s = (float) (0.55 * sw + 0.35 * sq) * 1.9f;
            v.modEnv *= kBassEnv;
            cutoff = (float) (260.0 + (700.0 + 3000.0 * br) * v.modEnv); reso = 0.45f; break;
        }
    }

    if ((v.tick++ & 7) == 0)
    {
        const float g = (float) std::tan (pi * std::min ((double) cutoff, sr * 0.45) / sr), k = 2.f - 1.9f * reso;
        v.a1 = 1 / (1 + g * (g + k)); v.a2 = g * v.a1; v.a3 = g * v.a2;
    }
    const float v3 = s - v.ic2, v1 = v.a1 * v.ic1 + v.a2 * v3, v2 = v.ic2 + v.a2 * v.ic1 + v.a3 * v3;
    v.ic1 = 2 * v1 - v.ic1; v.ic2 = 2 * v2 - v.ic2;
    float out = v2 * v.env * (0.4f + 0.6f * v.vel);

    // mouvement stéréo lent + trémolo (Rhodes)
    v.panPh += 0.05 * invSr;
    float pan = v.pan + 0.15f * (float) std::sin (twoPi * v.panPh);
    if (prm.tremolo > 0 && prm.model == Model::Rhodes) pan += prm.tremolo * 0.6f * (float) std::sin (twoPi * 4.6 * v.t);
    pan = std::clamp (pan, -1.f, 1.f);
    l += out * std::sqrt (0.5f * (1 - pan)) * 1.41f;
    r += out * std::sqrt (0.5f * (1 + pan)) * 1.41f;
}

// ---------------------------------------------------------------------------
void SmokeEngine::render (float* outL, float* outR, int n)
{
    aInc = (float) (1.0 / (std::max (0.5f, prm.attackMs) * 0.001 * sr));
    dCoef = t60 (prm.decayMs * 0.001, sr); rCoef = t60 (prm.releaseMs * 0.001, sr);
    kRhodesMod = t60 (1.2, sr); kRhodesTine = t60 (0.08, sr); kBassEnv = t60 (0.35, sr);
    for (int k = 0; k < 8; ++k) feltCoef[(size_t) k] = t60 (prm.decayMs * 0.001 / (1.0 + 0.45 * (k + 1)), sr);
    const float smA = onePole (15, sr), upA = onePole (40, sr), downA = onePole (2.5, sr);
    const float levelT = std::pow (10.f, prm.levelDb / 20.f);
    const float drive = 1.f + 3.5f * (1.f - std::clamp (prm.clean, 0.f, 1.f));
    const float driveNorm = 1.f / std::tanh (drive * 0.6f) * 0.6f;
    const int chSz = (int) chL.size(), preSz = (int) pre.size();
    float peak = 0;

    for (int i = 0; i < n; ++i)
    {
        float l = 0, r = 0;
        for (auto& v : voices) if (v.active) renderVoice (v, l, r);
        l *= 0.45f; r *= 0.45f;

        // Clean : saturation douce (0) .. propre (1)
        if (drive > 1.01f) { l = std::tanh (l * drive * 0.6f) * driveNorm; r = std::tanh (r * drive * 0.6f) * driveNorm; }

        // Tone : passe-bas 2 pôles, de 350 Hz à 18 kHz
        toneSm += smA * (prm.tone - toneSm);
        if ((i & 15) == 0) toneA = onePole (350.0 * std::pow (2.0, toneSm * 5.7), sr);
        const float ta = toneA;
        toneL1 += ta * (l - toneL1); toneL2 += ta * (toneL1 - toneL2);
        toneR1 += ta * (r - toneR1); toneR2 += ta * (toneR1 - toneR2);
        l = toneL2; r = toneR2;

        // Mod : chorus stéréo (avec la dérive de hauteur des voix)
        if (prm.mod > 0.01f)
        {
            chL[(size_t) chW] = l; chR[(size_t) chW] = r;
            chLfo += 0.45 * invSr; if (chLfo >= 1) chLfo -= 1;
            auto tap = [&] (std::vector<float>& b, double msDelay) {
                double rp = chW - msDelay * 0.001 * sr; while (rp < 0) rp += chSz;
                const int i0 = (int) rp, i1 = (i0 + 1) % chSz; const float fr = (float) (rp - i0);
                return b[(size_t) i0] + fr * (b[(size_t) i1] - b[(size_t) i0]); };
            const float dl = tap (chL, 12 + 5 * std::sin (twoPi * chLfo)), dr = tap (chR, 14 + 5 * std::cos (twoPi * chLfo));
            chW = (chW + 1) % chSz;
            l += prm.mod * 0.6f * (dr - 0.3f * l); r += prm.mod * 0.6f * (dl - 0.3f * r);
        }

        const float dry = 0.5f * (std::abs (l) + std::abs (r));
        follower += (dry > follower ? upA : downA) * (dry - follower);

        // Space + Dream : réverbération, Dream allonge la queue et ajoute un pré-délai
        spaceSm += smA * (prm.space - spaceSm); dreamSm += smA * (prm.dream - dreamSm);
        if (spaceSm > 0.001f || dreamSm > 0.001f)
        {
            pre[(size_t) preW] = (l + r) * 0.5f * 0.03f;
            int rp = preW - (int) ((0.005 + 0.09 * dreamSm) * sr); while (rp < 0) rp += preSz;
            const float in = pre[(size_t) rp]; preW = (preW + 1) % preSz;
            const float fbk = 0.80f + 0.185f * dreamSm, damp = 0.35f - 0.25f * dreamSm;
            float rl = 0, rr = 0;
            for (int c = 0; c < 8; ++c)
            {
                auto& cl = combL[(size_t) c]; auto& cr = combR[(size_t) c];
                float y = cl.b[(size_t) cl.i]; cl.store = y * (1 - damp) + cl.store * damp; cl.b[(size_t) cl.i] = in + cl.store * fbk; if (++cl.i >= (int) cl.b.size()) cl.i = 0; rl += y;
                y = cr.b[(size_t) cr.i]; cr.store = y * (1 - damp) + cr.store * damp; cr.b[(size_t) cr.i] = in + cr.store * fbk; if (++cr.i >= (int) cr.b.size()) cr.i = 0; rr += y;
            }
            for (int a = 0; a < 4; ++a)
            {
                auto& al = apL[(size_t) a]; auto& ar = apR[(size_t) a];
                float bo = al.b[(size_t) al.i]; al.b[(size_t) al.i] = rl + bo * 0.5f; rl = bo - rl; if (++al.i >= (int) al.b.size()) al.i = 0;
                bo = ar.b[(size_t) ar.i]; ar.b[(size_t) ar.i] = rr + bo * 0.5f; rr = bo - rr; if (++ar.i >= (int) ar.b.size()) ar.i = 0;
            }
            const float wet = std::min (1.f, spaceSm + 0.5f * dreamSm) * 1.5f;
            const float wa = 0.5f * (std::abs (rl) + std::abs (rr)) * wet;
            wetFollow += (wa > wetFollow ? upA : downA) * (wa - wetFollow);
            l = l * (1 - 0.3f * spaceSm) + rl * wet; r = r * (1 - 0.3f * spaceSm) + rr * wet;
        }

        levelSm += smA * (levelT - levelSm);
        l *= levelSm; r *= levelSm;
        auto safe = [] (float o) { const float a = std::abs (o);
            if (a > 0.9f) o = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), o);
            return std::isfinite (o) ? o : 0.f; };
        l = safe (l); r = safe (r);
        outL[i] = l; if (outR) outR[i] = r;
        peak = std::max ({ peak, std::abs (l), std::abs (r) });
    }
    fb.energy.store (std::min (1.f, follower * 3.f), std::memory_order_relaxed);
    const float fxAmount = 0.45f * prm.space + 0.35f * prm.dream + 0.25f * prm.mod + 0.2f * (1 - prm.clean);
    fb.intensity.store (std::min (1.f, fxAmount * std::min (1.f, (follower + wetFollow) * 4.f)), std::memory_order_relaxed);
    fb.held.store (heldCount, std::memory_order_relaxed);
    fb.voices.store (activeVoices(), std::memory_order_relaxed);
    fb.level.store (peak, std::memory_order_relaxed);
}
} // namespace smk
