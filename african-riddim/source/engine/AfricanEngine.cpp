#include "AfricanEngine.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>

namespace afr
{
namespace { constexpr double PI = 3.14159265358979323846; constexpr float TWO_PI = 6.2831853f; }

// gains de calibration (dB) : mesurés par tests/calibrate (même niveau perçu pour tous les instruments)
static const float kCalib[54] = {
#include "calibration.inc"
};
float Engine::calibrationDb (int i) { return kCalib[std::clamp (i, 0, 53)]; }

void Engine::Biquad::bandpass (double sr, double f, double q)
{
    f = std::clamp (f, 20.0, sr * 0.45); const double w = 2 * PI * f / sr, al = std::sin (w) / (2 * q), a0 = 1 + al;
    b0 = (float) (al / a0); b1 = 0; b2 = (float) (-al / a0); a1 = (float) (-2 * std::cos (w) / a0); a2 = (float) ((1 - al) / a0);
}
void Engine::Biquad::lowpass (double sr, double f, double q)
{
    f = std::clamp (f, 20.0, sr * 0.45); const double w = 2 * PI * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w), a0 = 1 + al;
    b0 = (float) ((1 - c) / 2 / a0); b1 = (float) ((1 - c) / a0); b2 = b0; a1 = (float) (-2 * c / a0); a2 = (float) ((1 - al) / a0);
}
void Engine::Biquad::highpass (double sr, double f, double q)
{
    f = std::clamp (f, 20.0, sr * 0.45); const double w = 2 * PI * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w), a0 = 1 + al;
    b0 = (float) ((1 + c) / 2 / a0); b1 = (float) (-(1 + c) / a0); b2 = b0; a1 = (float) (-2 * c / a0); a2 = (float) ((1 - al) / a0);
}

Engine::Engine() : voices ((size_t) kVoices) {}

void Engine::prepare (double s)
{
    sr = s;
    static const int base[kFdn] = { 1117, 1307, 1493, 1601, 1777, 1933, 2113, 2297 };
    for (int i = 0; i < kFdn; ++i) { fdnLen[i] = (int) (base[i] * sr / 48000.0); fdnBuf[i].assign ((size_t) fdnLen[i] + 1, 0.f); }
    preBuf.assign ((size_t) (sr * 0.05) + 1, 0.f);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices) { v.active = false; }
    for (int i = 0; i < kFdn; ++i) { std::fill (fdnBuf[i].begin(), fdnBuf[i].end(), 0.f); fdnPos[i] = 0; fdnLp[i] = 0; }
    std::fill (preBuf.begin(), preBuf.end(), 0.f); prePos = 0;
    toneZL = toneZR = 0; levelS = 0;
    for (auto& h : fb.held) h.store (0);
}

void Engine::setParams (const Params& p) { prm = p; prm.instrument = std::clamp (prm.instrument, 0, 53); }

void Engine::allNotesOff() { for (auto& v : voices) if (v.active) { v.held = false; v.releasing = true; } for (auto& h : fb.held) h.store (0); }

void Engine::noteOff (int note)
{
    fb.held[(size_t) (note & 127)].store (0);
    for (auto& v : voices) if (v.active && v.held && v.note == note) { v.held = false; v.releasing = true; }
}

void Engine::noteOn (int note, float vel)
{
    if (vel <= 0) { noteOff (note); return; }
    Voice* best = nullptr;
    // même note encore tenue sur un instrument à son continu : on la réutilise
    for (auto& v : voices) if (! v.active) { best = &v; break; }
    if (! best) { float oldest = -1; for (auto& v : voices) { const float sc = v.age + (v.held ? 0 : 100.f); if (sc > oldest) { oldest = sc; best = &v; } } }
    startVoice (*best, note, vel);
    fb.held[(size_t) (note & 127)].store (1);
    fb.hits.fetch_add (1); fb.lastVel.store (vel);
}

void Engine::startVoice (Voice& v, int note, float vel)
{
    const Inst& I = kInst[prm.instrument];
    v.active = true; v.held = true; v.releasing = false; v.note = note; v.inst = prm.instrument; v.vel = vel; v.age = 0;
    v.rng = 0x9E3779B9u ^ (uint32_t) (note * 7919 + ++stamp * 104729);
    const float n = (float) note + 12.f * (float) (I.octave + prm.octave) + prm.tune;
    v.freq = (float) (440.0 * std::pow (2.0, (n - 69.0) / 12.0));
    const float vg = (1 - prm.velSens) + prm.velSens * std::pow (vel, 1.4f);
    v.gain = vg * std::pow (10.f, kCalib[v.inst] / 20.f);
    v.pan = std::clamp ((float) (note - 60) / 48.f, -0.35f, 0.35f);
    const float atk = std::max (prm.attackMs, I.model == Model::Wind ? 35.f : I.model == Model::Bow ? 70.f : 0.f);
    v.atkInc = atk > 0.5f ? (float) (1.0 / (atk * 0.001 * sr)) : 1.f;
    v.env = v.atkInc >= 1 ? 1.f : 0.f;
    v.relMul = (float) std::exp (-6.9 / (std::max (5.f, prm.releaseMs) * 0.001 * sr));
    v.body.clear(); v.body2.clear(); v.res.clear(); v.buzzBp.clear();
    v.buzzEnv = 0; v.phase = v.phase2 = v.phaseDr = 0; v.noiseLp = 0; v.vib = 0; v.jitter = 0; v.exLp = 0; v.dcX = v.dcY = 0;
    const float bright = std::clamp (I.bright * (0.75f + 0.5f * vel), 0.f, 1.f);
    if (I.body > 0) v.body.bandpass (sr, I.body, I.bodyQ);
    v.buzzBp.bandpass (sr, std::min (5000.0, std::max (1800.0, v.freq * 6.0)), 1.2);
    v.nModes = 0;

    auto setModes = [&] (const float* ratios, const float* amps, const float* decs, int count, float baseF, float decay)
    {
        v.nModes = 0;
        for (int k = 0; k < count; ++k)
        {
            const float f = baseF * ratios[k]; if (f > sr * 0.45) break;
            Mode& m = v.modes[(size_t) v.nModes++];
            m.f = f; m.a = amps[k];
            const float t60 = std::max (0.01f, decay * decs[k] * std::pow (261.6f / std::max (40.f, baseF), 0.35f));
            m.d = (float) std::exp (-6.9 / (t60 * sr));
            m.c = (float) std::cos (2 * PI * f / sr); m.s = (float) std::sin (2 * PI * f / sr);
            m.re = 0; m.im = 0;
        }
    };

    switch (I.model)
    {
        case Model::Pluck: case Model::MouthBow:
        {
            const float b = I.model == Model::MouthBow ? 0.3f : bright;
            auto initString = [&] (float* buf, int& w, float& delay, float& apC, float& apZ, float& lpZ, float f)
            {
                const float N = (float) sr / f;
                // perte par période (T60) et amortissement des aigus
                v.lpA = std::clamp (0.62f - 0.55f * b, 0.04f, 0.7f);
                // retards de phase EXACTS à la fondamentale (passe-bas + passe-tout) : la corde est juste à toutes les hauteurs
                const double wf = 2 * PI * f / sr, a = v.lpA;
                const double phaseLp = std::atan2 (a * std::sin (wf), 1 - a * std::cos (wf)) / wf;
                double target = std::clamp ((double) N - phaseLp, 3.0, (double) kMaxDelay - 4);
                const double whole = std::floor (target - 0.5); const double frac = target - whole;
                double c = (1 - frac) / (1 + frac);
                for (int it = 0; it < 6; ++it)
                {   // H(z) = (c + z⁻¹)/(1 + c z⁻¹) : retard de phase -arg(H)/w, ajusté par la sécante
                    auto apDelay = [&] (double cc) { const std::complex<double> z = std::polar (1.0, -wf); return -std::arg ((cc + z) / (1.0 + cc * z)) / wf; };
                    const double d0 = apDelay (c), d1 = apDelay (c + 1e-4); const double slope = (d1 - d0) / 1e-4;
                    if (std::abs (slope) < 1e-9) break; c = std::clamp (c - (d0 - frac) / slope, -0.95, 0.95);
                }
                apC = (float) c; delay = (float) whole; apZ = 0; lpZ = 0;
                std::memset (buf, 0, sizeof (float) * kMaxDelay); w = 0;
                // excitation : bruit filtré + peigne de position d'attaque
                const int L = std::max (2, (int) std::lround (N));
                const int P = std::max (1, (int) (L * I.pos));
                float lp = 0; const float k = 0.15f + 0.8f * b;
                for (int i = 0; i < L && i < kMaxDelay; ++i) { lp += k * (v.noise() - lp); buf[i] = lp; }
                for (int i = L - 1; i >= P; --i) buf[i] -= buf[i - P];
                float mx = 0; for (int i = 0; i < L; ++i) mx = std::max (mx, std::abs (buf[i]));
                if (mx > 0) for (int i = 0; i < L; ++i) buf[i] /= mx;
                w = L % kMaxDelay;
            };
            const float dec = I.decay * std::pow (261.6f / std::max (40.f, v.freq), 0.45f);
            v.loss = (float) std::pow (10.0, -3.0 / (std::max (0.05f, dec) * v.freq));
            initString (v.dl, v.w, v.delay, v.apC, v.apZ, v.lpZ, v.freq);
            if (I.detune > 0) initString (v.dl2, v.w2, v.delay2, v.ap2C, v.ap2Z, v.lp2Z, v.freq * std::pow (2.f, I.detune / 1200.f));
            if (I.model == Model::MouthBow) v.res.bandpass (sr, v.freq * 3, 9);
            break;
        }
        case Model::Lamella:
        {
            const float r[] = { 1.f, 5.3f + I.pos, 12.4f, 0.5f }, a[] = { 1.f, 0.32f * bright, 0.10f * bright, 0.12f }, d[] = { 1.f, .22f, .08f, .3f };
            setModes (r, a, d, 4, v.freq, I.decay);
            v.exciteLeft = (int) (0.004 * sr);
            break;
        }
        case Model::Bar:
        {
            if (I.special == 1)        // gbi : tambour à fente en bois
            { const float r[] = { 1.f, 2.32f, 3.9f }, a[] = { 1.f, .45f, .12f }, d[] = { 1.f, .45f, .2f }; setModes (r, a, d, 3, v.freq, I.decay); }
            else if (I.special == 2)   // agogo : cloche
            { const float r[] = { 1.f, 2.71f, 5.18f, 8.06f, 11.6f }, a[] = { 1.f, .55f, .35f, .2f, .08f }, d[] = { 1.f, .7f, .45f, .3f, .2f }; setModes (r, a, d, 5, v.freq, I.decay); }
            else                       // lames accordées (balafon, timbila, silimba)
            { const float r[] = { 1.f, 3.93f, 9.0f, 0.5f }, a[] = { 1.f, .3f * bright, .08f * bright, .05f }, d[] = { 1.f, .3f, .12f, .4f }; setModes (r, a, d, 4, v.freq, I.decay); }
            v.exciteLeft = (int) ((I.special == 2 ? 0.0015 : 0.003) * sr);
            break;
        }
        case Model::Drum:
        {
            // la hauteur suit la touche, dans un registre de tambour
            const float f0 = std::clamp (v.freq * 0.5f, 40.f, 900.f);
            const float r[] = { 1.f, 1.59f, 2.14f, 2.30f, 2.65f, 2.92f, 3.16f, 3.50f };
            const float a[] = { 1.f, .6f, .45f, .35f, .3f, .22f, .16f, .12f };
            const float d[] = { 1.f, .55f, .42f, .35f, .3f, .25f, .2f, .16f };
            const int cnt = I.special == 3 ? 3 : I.special == 5 ? 6 : 8;
            setModes (r, a, d, cnt, f0, I.decay);
            v.pitchEnv = I.special == 3 ? -7.f : I.special == 1 ? 0.f : 2.5f * vel;
            v.pitchDrop = (float) std::exp (-1.0 / ((I.special == 3 ? 0.15 : 0.03) * sr));
            v.slap = I.pos * (0.4f + 0.8f * vel);
            v.exciteLeft = (int) ((0.002 + 0.004 * (1 - I.bright)) * sr);
            v.res.highpass (sr, 1800, 0.7);
            v.fric = 0;
            if (I.special == 4) { v.res.bandpass (sr, f0 * 2.2, 3); v.exciteLeft = 0; }
            break;
        }
        case Model::Wind: case Model::Bow:
        {
            if (I.model == Model::Bow) { v.body.bandpass (sr, 520, 1.8); v.body2.bandpass (sr, 1650, 2.5); v.res.bandpass (sr, 2900, 3); }
            else { v.body2.bandpass (sr, I.body * 1.9, 2.5); v.res.lowpass (sr, 800 + 9000 * bright, 0.7); }
            v.phaseDr = 0.5f; v.fric = 0;
            if (I.special == 2) v.pitchEnv = -1.2f;                   // kakaki : la note arrive par en dessous
            v.pitchDrop = (float) std::exp (-1.0 / (0.06 * sr));
            break;
        }
    }
}

// une étape de corde : lecture du retard, passe-tout fractionnaire, perte, passe-bas, grésillement
inline float Engine::stringStep (Voice& v, float* buf, int& w, float delay, float& apC, float& apZ, float& lpZ, float input)
{
    int r = w - (int) delay; if (r < 0) r += kMaxDelay;
    const float x = buf[r];
    // passe-tout du 1er ordre : y = c·x + z ; z = x − c·y
    const float y = apC * x + apZ; apZ = x - apC * y;
    lpZ += (1 - v.lpA) * (y - lpZ);
    float s = lpZ * v.loss + input;
    const Inst& I = kInst[v.inst];
    if (I.buzz > 0)
    {   // chevalet qui grésille : un côté de l'oscillation est écrêté doucement
        const float th = 0.06f + 0.5f * (1 - I.buzz);
        if (s > th) s = th + (s - th) * 0.35f;
    }
    buf[w] = s; w = (w + 1) % kMaxDelay;
    return y;
}

float Engine::renderVoice (Voice& v, float vibLfo)
{
    const Inst& I = kInst[v.inst];
    float out = 0;
    const float bendMul = std::pow (2.f, bend / 12.f);
    switch (I.model)
    {
        case Model::Pluck: case Model::MouthBow:
        {
            float s = stringStep (v, v.dl, v.w, v.delay, v.apC, v.apZ, v.lpZ, 0);
            if (I.detune > 0) s = 0.6f * s + 0.6f * stringStep (v, v.dl2, v.w2, v.delay2, v.ap2C, v.ap2Z, v.lp2Z, 0);
            if (I.model == Model::MouthBow)
            {   // la bouche / la calebasse fait ressortir un harmonique qui se déplace lentement
                v.phase2 += (float) ((I.special ? 0.18 : 0.35) / sr); if (v.phase2 > 1) v.phase2 -= 1;
                const float h = 2.5f + 2.f * (0.5f + 0.5f * std::sin (TWO_PI * v.phase2));
                if (((int) (v.age * sr)) % 64 == 0) v.res.bandpass (sr, std::min ((double) v.freq * h, sr * 0.4), 10);
                s = 0.35f * s + 1.6f * v.res.run (s);
            }
            else
            {
                if (I.buzz > 0) { v.buzzEnv += 0.01f * (std::abs (s) - v.buzzEnv); s += I.buzz * 0.5f * v.buzzBp.run (v.noise() * v.buzzEnv * v.buzzEnv * 3.f); }
                if (I.body > 0) s += I.bodyGain * v.body.run (s) * 1.5f;
            }
            out = s * 0.55f;
            break;
        }
        case Model::Lamella: case Model::Bar: case Model::Drum:
        {
            float ex = 0;
            if (v.exciteLeft > 0) { --v.exciteLeft; v.exLp += 0.35f * (v.noise() - v.exLp); ex = v.exLp * 0.08f; }
            float fMul = bendMul;
            if (I.model == Model::Drum)
            {
                v.pitchEnv *= v.pitchDrop;
                float talk = 0;
                if (I.special == 1)    // tambour parlant : la hauteur monte puis redescend (pression des cordes)
                    talk = 3.5f * v.vel * std::exp (-v.age * 3.f) * std::min (1.f, v.age * 18.f) + bend * 2.f;
                fMul = std::pow (2.f, (v.pitchEnv + talk) / 12.f) * (I.special == 1 ? 1.f : bendMul);
            }
            const bool start = v.age == 0;
            for (int k = 0; k < v.nModes; ++k)
            {
                Mode& m = v.modes[(size_t) k];
                float c = m.c, s = m.s;
                if (fMul != 1.f) { const float w = TWO_PI * m.f * fMul / (float) sr; c = std::cos (w); s = std::sin (w); }
                if (start) m.re += m.a * (I.model == Model::Drum ? 0.6f : 0.35f);
                m.re += ex * m.a;
                const float re = (m.re * c - m.im * s) * m.d, im = (m.re * s + m.im * c) * m.d;
                m.re = re; m.im = im; out += im;
            }
            if (I.model == Model::Drum)
            {
                if (v.slap > 0.001f && v.age < 0.03f) { out += v.res.run (v.noise()) * v.slap * std::exp (-v.age * 160.f) * 0.8f; }
                if (I.special == 4 && v.held)
                {   // puita : frottement d'une baguette sur la peau (son tenu, grinçant)
                    v.phase += v.freq * 0.5f * (1 + 0.02f * v.noise()) / (float) sr; if (v.phase > 1) v.phase -= 1;
                    const float pulse = v.phase < 0.08f ? 1.f : -0.087f;
                    out += v.res.run (pulse) * 0.7f;
                }
                if (I.buzz > 0) { v.buzzEnv += 0.02f * (std::abs (out) - v.buzzEnv); out += I.buzz * 0.4f * v.buzzBp.run (v.noise() * v.buzzEnv * 2.f); }
            }
            else
            {
                if (ex != 0) out += ex * (I.model == Model::Bar ? 2.f : 1.f);    // le choc de la mailloche / du pouce
                if (I.buzz > 0)
                {   // anneaux métalliques (sanza) ou membrane de mirliton (balafon) : grésillement qui suit le son
                    v.buzzEnv += 0.01f * (std::abs (out) - v.buzzEnv);
                    out += I.buzz * v.buzzBp.run (std::tanh (out * 6.f) * 0.25f + v.noise() * v.buzzEnv * 1.2f) * 0.8f;
                }
                if (I.body > 0) out += I.bodyGain * v.body.run (out) * 1.2f;
            }
            out *= 0.6f;
            break;
        }
        case Model::Wind: case Model::Bow:
        {
            v.pitchEnv *= v.pitchDrop;
            const float vibAmt = (0.003f + 0.012f * mod) * std::min (1.f, v.age * 2.5f);
            v.jitter += 0.002f * (v.noise() - v.jitter);
            const float f = v.freq * bendMul * std::pow (2.f, v.pitchEnv / 12.f) * (1 + vibAmt * vibLfo + 0.004f * v.jitter);
            auto saw = [&] (float& ph, float fr) { const float dt = fr / (float) sr; ph += dt; if (ph >= 1) ph -= 1;
                float y = 2 * ph - 1; // polyBLEP
                if (ph < dt) { const float t = ph / dt; y -= t + t - t * t - 1; } else if (ph > 1 - dt) { const float t = (ph - 1) / dt; y -= t * t + t + t + 1; }
                return y; };
            const float br = v.noise();
            v.noiseLp += 0.2f * (br - v.noiseLp);
            if (I.model == Model::Bow)
            {
                float s = saw (v.phase, f) * (0.85f + 0.15f * v.jitter);
                s = 0.25f * s + v.body.run (s) * 1.6f + v.body2.run (s) * 1.1f + v.res.run (s) * 0.6f;
                s += I.buzz * 0.08f * v.noiseLp;
                out = s * 0.5f;
            }
            else if (I.special == 0 && I.pos > 0.5f)          // gasba : flûte à embouchure terminale
            {
                v.phase += f / (float) sr; if (v.phase >= 1) v.phase -= 1;
                const float s = std::sin (TWO_PI * v.phase) + 0.12f * std::sin (2 * TWO_PI * v.phase);
                const float breath = v.body2.run (br) * (0.35f + 0.6f * std::exp (-v.age * 12.f));
                out = (s * 0.8f + breath * 0.9f) * 0.5f;
            }
            else
            {
                // anche : deux dents de scie décalées d'une demi-période = timbre « creux » (harmoniques impaires) ; 2e tuyau légèrement désaccordé
                float a = saw (v.phase, f) - saw (v.phaseDr, f) ;
                float b = 0;
                if (I.detune > 0) { const float f2 = f * std::pow (2.f, I.detune / 1200.f); v.phase2 += f2 / (float) sr; if (v.phase2 >= 1) v.phase2 -= 1; b = 2 * v.phase2 - 1; }
                float s = 0.7f * a + 0.35f * b;
                if (I.special == 1) { v.fric += 0.5f * f / (float) sr; if (v.fric >= 1) v.fric -= 1; s += 0.35f * std::sin (TWO_PI * v.fric); }           // mezoued : bourdon une octave sous la note
                if (I.special == 2) s *= 1 + 0.25f * v.jitter * 8.f;                                                // kakaki : grognement
                if (I.special == 3) s = std::tanh (s * 2.5f);                                                       // lesiba : arc soufflé, plus rugueux
                s = v.res.run (s);
                s = 0.5f * s + v.body2.run (s) * 1.2f + (I.body > 0 ? v.body.run (s) * I.bodyGain : 0);
                s += 0.05f * v.noiseLp;
                out = s * 0.4f;
            }
            break;
        }
    }
    // enveloppe
    if (v.env < 1 && ! v.releasing) v.env = std::min (1.f, v.env + v.atkInc);
    if (v.releasing)
    {
        const bool oneShot = I.model == Model::Drum && I.special != 4;
        if (! oneShot) v.env *= v.relMul;
    }
    v.age += (float) (1.0 / sr);
    out *= v.env * v.gain;
    // fin de voix : silence durable
    v.buzzEnv = std::isfinite (v.buzzEnv) ? v.buzzEnv : 0;
    v.dcY = 0.999f * v.dcY + 0.001f * std::abs (out);
    if ((v.env < 1e-4f) || (v.age > 0.2f && v.dcY < 2e-6f)) { v.active = false; }
    return out;
}

void Engine::process (float* L, float* R, int n)
{
    const float volT = std::pow (10.f, prm.volumeDb / 20.f);
    limRel = (float) (1 - std::exp (-1.0 / (0.15 * sr)));
    // réverbération : durée et mélange
    const float rt = 1.2f + 1.6f * prm.space;
    const float wet = prm.space * 0.55f;
    const float toneA = (float) std::exp (-2 * PI * 1800 / sr);
    const float hiG = 0.35f + 1.3f * prm.tone, loG = 1.15f - 0.3f * prm.tone;
    int act = 0;
    for (int i = 0; i < n; ++i)
    {
        lfo += (float) (5.2 / sr); if (lfo > 1) lfo -= 1;
        const float vibLfo = std::sin (TWO_PI * lfo);
        float l = 0, r = 0;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const float s = renderVoice (v, vibLfo);
            l += s * (0.5f - 0.5f * v.pan) * 1.41f; r += s * (0.5f + 0.5f * v.pan) * 1.41f;
        }
        // couleur
        toneZL += (1 - toneA) * (l - toneZL); toneZR += (1 - toneA) * (r - toneZR);
        l = toneZL * loG + (l - toneZL) * hiG; r = toneZR * loG + (r - toneZR) * hiG;
        // réverbération FDN (Householder)
        float wl = 0, wr = 0;
        if (wet > 0.001f)
        {
            const int pd = (int) preBuf.size(); preBuf[(size_t) prePos] = 0.5f * (l + r);
            const float in = preBuf[(size_t) ((prePos + 1) % pd)]; prePos = (prePos + 1) % pd;
            float o[kFdn], sum = 0;
            for (int k = 0; k < kFdn; ++k) { o[k] = fdnBuf[k][(size_t) fdnPos[k]]; sum += o[k]; }
            sum *= 2.f / kFdn;
            for (int k = 0; k < kFdn; ++k)
            {
                const float g = (float) std::pow (10.0, -3.0 * fdnLen[k] / (rt * sr));
                float x = (o[k] - sum) * g;
                fdnLp[k] += 0.55f * (x - fdnLp[k]); x = fdnLp[k];
                fdnBuf[k][(size_t) fdnPos[k]] = x + in * 0.35f;
                fdnPos[k] = (fdnPos[k] + 1) % fdnLen[k];
            }
            wl = (o[0] + o[2] + o[4] + o[6]) * 0.5f; wr = (o[1] + o[3] + o[5] + o[7]) * 0.5f;
        }
        volS += 0.002f * (volT - volS);
        float yl = (l + wl * wet) * volS, yr = (r + wr * wet) * volS;
        // limiteur de sécurité (-1 dBFS) : transparent en jeu normal, évite la saturation quand on empile les notes
        const float pk = std::max (std::abs (yl), std::abs (yr));
        const float need = pk > 0.891f ? 0.891f / pk : 1.f;
        limG = need < limG ? need : limG + (1 - limG) * limRel;
        yl *= limG; yr *= limG;
        if (! std::isfinite (yl) || ! std::isfinite (yr)) { yl = yr = 0; reset(); }
        L[i] = yl; R[i] = yr;
        levelS = std::max (std::abs (yl), levelS * 0.9995f);
    }
    for (auto& v : voices) act += v.active ? 1 : 0;
    fb.voices.store (act); fb.level.store (levelS);
}
} // namespace afr
