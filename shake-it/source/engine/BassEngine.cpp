#include "BassEngine.h"
#include <algorithm>
#include <cmath>

namespace shk
{
namespace
{
constexpr double twoPi = 6.283185307179586;

inline double noteToHz (double semi) { return 440.0 * std::pow (2.0, (semi - 69.0) / 12.0); }

// t60 (temps pour perdre 60 dB) -> coefficient multiplicatif par échantillon
inline float t60Coef (double seconds, double sr)
{
    seconds = std::max (seconds, 0.001);
    return (float) std::exp (std::log (0.001) / (seconds * sr));
}

inline float onePoleCoef (double hz, double sr)
{
    return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr));
}

inline double polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}
} // namespace

void BassEngine::prepare (double sampleRate)
{
    sr = sampleRate > 1000.0 ? sampleRate : 48000.0;
    invSr = 1.0 / sr;
    ks.assign ((size_t) (sr / 15.0) + 8, 0.f);   // corde jusqu'à 15 Hz
    reset();
}

void BassEngine::reset()
{
    stackSize = 0;
    gate = false;
    ampEnv = 0.f;
    inAttack = false;
    phase = clickPhase = 0.0;
    glidePos = 1.0;
    lpState1 = lpState2 = 0.f;
    ld.fill (0.f);
    dcX = dcY = 0.f;
    bassLp = follower = 0.f;
    std::fill (ks.begin(), ks.end(), 0.f);
    ksWrite = 0;
    ksLast = 0.f;
    motion.energy.store (0.f);
    motion.glide.store (0.f);
    motion.note.store (-1);
    motion.level.store (0.f);
}

// ---------------------------------------------------------------------------
void BassEngine::noteOn (int note, float vel)
{
    // retire la note si elle est déjà dans la pile, puis l'empile en haut
    int w = 0;
    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] != note) stack[(size_t) w++] = stack[(size_t) i];
    stackSize = w;
    if (stackSize == (int) stack.size())
    {
        std::copy (stack.begin() + 1, stack.end(), stack.begin());
        --stackSize;
    }
    stack[(size_t) stackSize++] = note;

    if (gate) setTarget (note, true);          // legato : glisse, pas de nouvelle attaque
    else trigger (note, vel, false);
}

void BassEngine::noteOff (int note)
{
    int w = 0;
    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] != note) stack[(size_t) w++] = stack[(size_t) i];
    const bool wasTop = stackSize > 0 && stack[(size_t) stackSize - 1] == note;
    stackSize = w;

    if (! gate) return;
    if (stackSize == 0) gate = false;                               // relâchement
    else if (wasTop) setTarget (stack[(size_t) stackSize - 1], true); // retour à la note tenue
}

void BassEngine::allNotesOff()
{
    stackSize = 0;
    gate = false;
}

void BassEngine::setTarget (int note, bool glide)
{
    targetSemi = (double) note;
    if (glide && prm.glideMs > 0.5f && std::abs (targetSemi - curSemi) > 1.0e-6)
    {
        startSemi = curSemi;
        glidePos = 0.0;
        glideStep = 1.0 / (prm.glideMs * 0.001 * sr);
    }
    else
    {
        curSemi = startSemi = targetSemi;
        glidePos = 1.0;
    }
}

void BassEngine::trigger (int note, float vel, bool)
{
    const bool stillSounding = ampEnv > 1.0e-3f;
    gate = true;
    velocity = std::clamp (vel, 0.f, 1.f);
    setTarget (note, ! prm.glideLegatoOnly && stillSounding);

    tSinceTrigger = 0.0;
    clickPhase = 0.0;
    attackStart = ampEnv;          // repart du niveau actuel : pas de clic
    inAttack = true;
    attackPos = 0.0;
    if (! stillSounding)
    {
        phase = 0.0;
        ld.fill (0.f);
    }

    if (prm.model == Model::Pluck)
    {
        // excitation : bruit filtré sur une période, dans le tampon préalloué
        const double hz = noteToHz (curSemi + prm.tune);
        const int len = std::clamp ((int) (sr / hz), 2, (int) ks.size() - 4);
        float lp = 0.f;
        const float bright = 0.25f + 0.7f * std::clamp (prm.tone / 18000.f, 0.f, 1.f);
        for (int i = 0; i < len; ++i)
        {
            rng = rng * 1664525u + 1013904223u;
            const float nz = ((float) (rng >> 9) / 4194304.f) - 1.f;
            lp += bright * (nz - lp);
            int idx = ksWrite - len + i;
            while (idx < 0) idx += (int) ks.size();
            ks[(size_t) idx] = ks[(size_t) idx] * 0.3f + lp * 0.9f;
        }
    }

    motion.onsets.fetch_add (1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
float BassEngine::shape (float x) const
{
    switch (prm.driveType)
    {
        case DriveType::Hard: return std::clamp (x, -1.f, 1.f);
        case DriveType::Fold: return std::sin (x);
        case DriveType::Soft:
        default:              return std::tanh (x);
    }
}

float BassEngine::ladder (float x, float cutoffHz, float res)
{
    const float g = onePoleCoef (cutoffHz, sr);
    const float fb = res * 3.9f;
    float in = std::tanh (x - fb * ld[3]);
    for (auto& s : ld)
    {
        s += g * (in - s);
        in = s;
    }
    return ld[3] * (1.4f + res * 2.2f);
}

// ---------------------------------------------------------------------------
void BassEngine::render (float* outL, float* outR, int n)
{
    // coefficients du bloc (calculés une fois, pas par échantillon)
    const float decayCoef = t60Coef (prm.decay, sr);
    const float relCoef = t60Coef (prm.releaseMs * 0.001, sr);
    const double attackInc = 1.0 / (std::max (0.5f, prm.attackMs) * 0.001 * sr);
    const float toneA = onePoleCoef (prm.tone, sr);
    const float bassA = onePoleCoef (150.0, sr);
    const float folUp = onePoleCoef (1.0 / (twoPi * 0.010), sr);
    const float folDown = onePoleCoef (1.0 / (twoPi * 0.120), sr);
    const float targetGain = std::pow (10.f, prm.outputDb / 20.f);
    const float gainA = onePoleCoef (30.0, sr);
    const float drv = std::clamp (prm.drive, 0.f, 1.f);
    const float drvGain = 1.f + drv * drv * 24.f + drv * 3.f;
    const float drvComp = 1.f - 0.3f * drv;
    const float velScale = 0.55f + 0.45f * velocity;
    const float punch = std::clamp (prm.punch, 0.f, 1.f);
    const float harm = std::clamp (prm.harm, 0.f, 1.f);
    const double pitchTau = 0.004 + (1.0 - punch) * 0.035;
    const double pitchDepth = prm.model == Model::Bass808 ? punch * 24.0
                            : prm.model == Model::Sub ? punch * 5.0 : 0.0;
    const float res = std::clamp (prm.reso, 0.f, 0.97f);

    float peak = 0.f;

    for (int i = 0; i < n; ++i)
    {
        // ---- hauteur (glide linéaire en demi-tons)
        if (glidePos < 1.0)
        {
            glidePos = std::min (1.0, glidePos + glideStep);
            curSemi = startSemi + (targetSemi - startSemi) * glidePos;
        }
        tSinceTrigger += invSr;
        const double pEnv = pitchDepth > 0.0 ? pitchDepth * std::exp (-tSinceTrigger / pitchTau) : 0.0;
        const double hz = std::min (noteToHz (curSemi + prm.tune + pEnv), sr * 0.45);
        curHz = hz;
        const double dt = hz * invSr;

        // ---- enveloppe d'amplitude
        if (inAttack)
        {
            attackPos += attackInc;
            if (attackPos >= 1.0) { attackPos = 1.0; inAttack = false; }
            ampEnv = attackStart + (1.f - attackStart) * (float) attackPos;
            if (! gate) inAttack = false;
        }
        else
            ampEnv *= gate ? decayCoef : relCoef;
        if (ampEnv < 1.0e-7f) ampEnv = 0.f;

        // ---- source selon le modèle
        float s = 0.f;
        switch (prm.model)
        {
            case Model::Bass808:
            {
                const double ph = phase * twoPi;
                s = (float) std::sin (ph);
                if (harm > 0.f)
                    s += harm * (float) (0.45 * std::sin (2.0 * ph) + 0.22 * std::sin (3.0 * ph));
                if (tSinceTrigger < 0.012 && punch > 0.f)
                {
                    clickPhase += 1700.0 * invSr;
                    s += punch * 0.35f * (float) (std::exp (-tSinceTrigger / 0.0016) * std::sin (clickPhase * twoPi));
                }
                s = s / (1.f + 0.4f * harm);
                break;
            }
            case Model::Sub:
            {
                const double ph = phase * twoPi;
                s = (float) std::sin (ph);
                if (harm > 0.f)
                    s += harm * (float) (0.3 * std::sin (2.0 * ph) + 0.12 * std::sin (4.0 * ph));
                s = s / (1.f + 0.25f * harm);
                break;
            }
            case Model::Moog:
            {
                double saw = 2.0 * phase - 1.0 - polyBlep (phase, dt);
                double t2 = phase + 0.5; if (t2 >= 1.0) t2 -= 1.0;
                double sq = (phase < 0.5 ? 1.0 : -1.0) + polyBlep (phase, dt) - polyBlep (t2, dt);
                const float raw = (float) ((1.0 - harm) * saw + harm * sq) * 0.7f;
                const float fenv = punch * 5.f * (float) std::exp (-tSinceTrigger / 0.12);
                const float cutoff = std::min (prm.tone * std::pow (2.f, fenv), (float) (sr * 0.45));
                s = ladder (raw, cutoff, res);
                break;
            }
            case Model::Pluck:
            {
                const double len = std::clamp (sr / hz, 2.0, (double) ks.size() - 4.0);
                double rp = (double) ksWrite - len;
                while (rp < 0.0) rp += (double) ks.size();
                const int i0 = (int) rp;
                const int i1 = (i0 + 1) % (int) ks.size();
                const float fr = (float) (rp - i0);
                const float y = ks[(size_t) i0] + fr * (ks[(size_t) i1] - ks[(size_t) i0]);
                const float avg = 0.5f * (y + ksLast);
                const float damp = std::clamp (prm.tone / 18000.f, 0.05f, 1.f);
                const float loop = (float) std::pow (0.001, 1.0 / (std::max (0.05f, prm.decay) * hz));
                ksLast = y;
                ks[(size_t) ksWrite] = (damp * y + (1.f - damp) * avg) * loop;
                ksWrite = (ksWrite + 1) % (int) ks.size();
                s = y * 1.4f;
                if (harm > 0.f) s = std::tanh (s * (1.f + 3.f * harm)) / (1.f + harm);
                break;
            }
        }
        phase += dt;
        if (phase >= 1.0) phase -= 1.0;

        float x = s * ampEnv * velScale;

        // ---- Tone (sauf Moog qui a son propre filtre)
        if (prm.model != Model::Moog && prm.model != Model::Pluck)
        {
            lpState1 += toneA * (x - lpState1);
            lpState2 += toneA * (lpState1 - lpState2);
            x = lpState2;
        }

        // ---- Drive
        if (drv > 0.001f)
            x = shape (x * drvGain) * drvComp;

        // ---- anti-continu (5 Hz)
        const float y = x - dcX + 0.99935f * dcY;
        dcX = x; dcY = y;

        // ---- sortie + limiteur doux de sécurité (jamais au-dessus de 0 dBFS)
        outGainSmoothed += gainA * (targetGain - outGainSmoothed);
        float o = y * outGainSmoothed;
        const float a = std::abs (o);
        if (a > 0.9f) o = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), o);
        if (! std::isfinite (o)) o = 0.f;

        outL[i] = o;
        if (outR != nullptr) outR[i] = o;
        peak = std::max (peak, std::abs (o));

        // ---- énergie du grave pour l'animation
        bassLp += bassA * (o - bassLp);
        const float r = std::abs (bassLp);
        follower += (r > follower ? folUp : folDown) * (r - follower);
    }

    motion.energy.store (std::min (1.5f, follower * 1.6f), std::memory_order_relaxed);
    motion.glide.store (glidePos < 1.0 ? (float) (1.0 - glidePos) : 0.f, std::memory_order_relaxed);
    motion.note.store (gate && stackSize > 0 ? stack[(size_t) stackSize - 1] : (ampEnv > 0.01f ? (int) targetSemi : -1),
                       std::memory_order_relaxed);
    motion.level.store (peak, std::memory_order_relaxed);
}
} // namespace shk
