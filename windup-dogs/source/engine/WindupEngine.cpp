#include "WindupEngine.h"
#include <algorithm>
#include <cmath>

namespace wd
{
namespace
{
constexpr double twoPi = 6.283185307179586, pi = 3.141592653589793;
inline float t60 (double s, double sr) { return (float) std::exp (std::log (0.001) / (std::max (s, 0.0005) * sr)); }
inline float onePole (double hz, double sr) { return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr)); }
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
inline double wrap (double p) { return p - std::floor (p); }
const double kDiv[4] = { 1.0, 0.5, 0.25, 1.0 / 3.0 };
} // namespace

void WindupEngine::Svf::set (double hz, double q, double sr)
{
    const double g = std::tan (pi * std::min (hz, sr * 0.45) / sr);
    k = (float) (1.0 / std::max (0.3, q));
    a1 = (float) (1.0 / (1.0 + g * (g + k))); a2 = (float) (g * a1); a3 = (float) (g * a2);
}

void WindupEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1000 ? sampleRate : 48000; invSr = 1.0 / sr;
    dlL.assign ((size_t) (sr * 1.5) + 8, 0.f); dlR.assign (dlL.size(), 0.f);
    static const int combT[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }, apT[4] = { 556, 441, 341, 225 };
    const double k = sr / 44100.0 * 0.9;     // petite salle : jouet dans une chambre
    for (int i = 0; i < 8; ++i) { combL[(size_t) i].b.assign ((size_t) (combT[i] * k) + 1, 0.f); combR[(size_t) i].b.assign ((size_t) ((combT[i] + 23) * k) + 1, 0.f); }
    for (int i = 0; i < 4; ++i) { apL[(size_t) i].b.assign ((size_t) (apT[i] * k) + 1, 0.f); apR[(size_t) i].b.assign ((size_t) ((apT[i] + 23) * k) + 1, 0.f); }
    reset();
}

void WindupEngine::reset()
{
    for (auto& v : voices) v = Voice {};
    for (auto& c : clicks) c = Click {};
    std::fill (dlL.begin(), dlL.end(), 0.f); std::fill (dlR.begin(), dlR.end(), 0.f); dlW = 0;
    for (auto* a : { &combL, &combR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; c.store = 0; }
    for (auto* a : { &apL, &apR }) for (auto& c : *a) { std::fill (c.b.begin(), c.b.end(), 0.f); c.i = 0; }
    heldCount = 0; follower = 0; toneL1 = toneL2 = toneR1 = toneR2 = 0; lastStep = -1; freeBeats = 0; windTarget = -1; windClicks = 0;
}

void WindupEngine::setParams (const Params& p)
{
    prm = p;
    prm.tickDiv = std::clamp (prm.tickDiv, 0, 3);
    prm.springTime = std::clamp (prm.springTime, 2.f, 600.f);
}
void WindupEngine::setTransport (double b, double q) { bpm = std::clamp (b > 1 ? b : 120.0, 30.0, 300.0); ppq = q; }
int WindupEngine::activeVoices() const { int n = 0; for (auto& v : voices) n += v.active; return n; }

// ---------------------------------------------------------------------------
void WindupEngine::addClick (double hz, double hz2, float amp, float ms, float pan, int delay)
{
    if (amp <= 0.0005f) return;
    Click* c = nullptr;
    for (auto& x : clicks) if (! x.on) { c = &x; break; }
    if (! c) { c = &clicks[0]; for (auto& x : clicks) if (x.env < c->env) c = &x; }
    *c = Click {};
    c->on = true; c->delay = delay; c->env = 1; c->coef = t60 (ms * 0.001, sr); c->amp = amp; c->pan = pan;
    c->a.set (hz, 14, sr); c->b.set (hz2, 9, sr);
}

void WindupEngine::wind()
{
    windTarget = 1.f; windClicks = 12; windNext = 0;
    fb.winds.fetch_add (1, std::memory_order_relaxed);
}

void WindupEngine::noteOn (int note, float vel)
{
    vel = std::clamp (vel, 0.f, 1.f);
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = std::min (w, (int) held.size() - 1);
    if (heldCount == 0 && activeVoices() == 0 && ppq < 0) { freeBeats = 0; lastStep = -1; }   // tic-tac libre : repart du début
    held[(size_t) heldCount++] = note;
    fb.noteHits.fetch_add (1, std::memory_order_relaxed);
    fb.lastNote.store (note, std::memory_order_relaxed);
    fb.lastVel.store (vel, std::memory_order_relaxed);
    startVoice (note, note, vel);
    // bruit mécanique de l'attaque (picot, marteau, piston…)
    const float m = prm.meca * vel, pan = std::clamp ((note - 60) / 40.f, -0.6f, 0.6f);
    switch (prm.inst)
    {
        case Inst::MusicBox: addClick (3600 + 900 * noise(), 6300, m * 0.45f, 3.f, pan); break;
        case Inst::ToyPiano: addClick (190, 560, m * 0.9f, 22.f, pan); addClick (2400, 4100, m * 0.22f, 3.f, pan); break;
        case Inst::Bells:    addClick (5200, 7600, m * 0.3f, 2.5f, pan); break;
        case Inst::Fanfare:  addClick (1300, 2500, m * 0.35f, 5.f, pan); break;
        case Inst::Drum: case Inst::Count: default: addClick (4600, 6900, m * 0.22f, 3.f, pan); break;
    }
}

void WindupEngine::noteOff (int note)
{
    int w = 0;
    for (int i = 0; i < heldCount; ++i) if (held[(size_t) i] != note) held[(size_t) w++] = held[(size_t) i];
    heldCount = w;
    for (auto& v : voices) if (v.active && ! v.released && v.key == note && v.inst != Inst::Drum) { v.released = true; v.stage = 3; }
    for (auto& v : voices) if (v.active && v.key == note && v.inst == Inst::Drum) v.released = true;
}

void WindupEngine::allNotesOff() { heldCount = 0; for (auto& v : voices) if (v.active) { v.released = true; v.stage = 3; } }

void WindupEngine::startVoice (int note, int key, float vel)
{
    if (note < 0 || note > 127) return;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (! v) { float best = 2; for (auto& x : voices) if (x.released && x.aenv < best) { best = x.aenv; v = &x; } }
    if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    *v = Voice {};
    v->active = true; v->note = note; v->key = key; v->vel = vel; v->age = ++ageCounter; v->inst = prm.inst;
    v->hz = 440.0 * std::pow (2.0, (note + prm.tune - 69.0) / 12.0);
    v->pan = std::clamp ((note - 60) / 40.f, -0.6f, 0.6f) + (noise() * 0.12f);
    const double br = std::clamp ((double) prm.bright, 0.0, 1.0), vb = 0.4 + 0.6 * vel;
    const double base = std::clamp (prm.decayMs * 0.001 * std::pow (440.0 / v->hz, 0.35), 0.05, 14.0);
    auto part = [&] (double ratio, double amp, double dec) {
        if (v->np >= 6 || v->hz * ratio >= sr * 0.45 || amp <= 0) return;
        const int k = v->np++; v->ratio[k] = ratio; v->amp[k] = (float) amp; v->env[k] = 1; v->coef[k] = t60 (base * dec, sr); v->ph[k] = wrap (0.13 * k); };
    switch (prm.inst)
    {
        case Inst::MusicBox:     // lame d'acier pincée par un picot : deux lames presque à l'unisson qui battent
            part (1.0, 1.0, 1.0); part (1.0021, 0.38, 0.9); part (3.0, 0.16 * vb, 0.25);
            part (5.93, 0.28 * br * vb, 0.06); part (9.1, 0.1 * br * vb, 0.03); break;
        case Inst::ToyPiano:     // tiges frappées : partiels inharmoniques courts, fondamental sec
            part (1.0, 1.0, 0.8); part (3.01, (0.12 + 0.35 * br) * vb, 0.28); part (5.41, 0.22 * br * vb, 0.12);
            part (8.93, 0.1 * br * vb, 0.05); part (0.5, 0.06, 0.35); break;
        case Inst::Bells:        // clochettes : partiels de cloche, longues queues, battements
            part (1.0, 1.0, 1.7); part (1.0035, 0.4, 1.5); part (2.0, 0.5, 1.1); part (2.76, 0.42 * vb, 0.8);
            part (4.07, 0.3 * (0.4 + 0.6 * br) * vb, 0.45); part (5.4, 0.22 * br * vb, 0.25); break;
        case Inst::Fanfare:
        {
            v->f1.set (1150 + 500 * br, 4.5, sr); v->f2.set (2600 + 900 * br, 5.5, sr); v->lp.set (std::min (sr * 0.45, v->hz * (4 + 10 * br)), 0.7, sr);
            break;
        }
        case Inst::Drum: case Inst::Count: default:
        {
            const int d = note % 12; v->drum = d;
            const double tr = std::pow (2.0, ((note - d) - 60) / 12.0 * 0.5);   // une octave plus haut = sons un peu plus aigus
            auto tone = [&] (double f0, double f1, double pitchS, double decS, float amp) { v->f0 = f0 * tr; v->f1hz = f1 * tr; v->pitchCoef = t60 (pitchS, sr); v->toneCoef = t60 (decS, sr); v->toneEnv = 1; v->toneAmp = amp; };
            auto nz = [&] (double hz, double q, double decS, float amp) { v->nf.set (hz * std::sqrt (tr), q, sr); v->noiseCoef = t60 (decS, sr); v->noiseEnv = 1; v->noiseAmp = amp; };
            switch (d)
            {
                case 0: tone (240, 190, 0.05, 0.11, 0.55f); nz (3300, 1.2, 0.16, 0.8f); break;                      // tambour en fer-blanc
                case 1: tone (920, 900, 0.02, 0.05, 1.f); part (1.52, 0.35, 1); v->coef[0] = t60 (0.04, sr); nz (2100, 2, 0.012, 0.15f); break;   // bloc de bois
                case 2: tone (180, 118, 0.12, 0.26, 1.f); nz (900, 1.5, 0.05, 0.12f); break;                        // tom jouet
                case 3: nz (2700, 3, 0.007, 0.9f); v->noiseEnv = 0; v->clicks = 9; v->clickGap = 0.027; v->toneEnv = 0; break;   // crécelle
                case 4: nz (6800, 0.8, 0.45, 0.45f); tone (930, 930, 1, 0.32, 0.2f); part (1.48, 0.8, 1); part (2.17, 0.6, 1); for (int k = 0; k < v->np; ++k) v->coef[k] = t60 (0.3, sr); break;   // petite cymbale
                case 5: nz (3100, 3, 0.012, 1.f); v->clicks = 1; v->clickGap = 0.034; break;                         // castagnettes
                case 6: tone (1850, 1850, 1, 1.4, 0.45f); part (2.76, 0.5, 1); part (5.4, 0.25, 1); for (int k = 0; k < v->np; ++k) v->coef[k] = t60 (1.1, sr); nz (9000, 1, 0.01, 0.1f); break;   // triangle
                case 7: tone (560, 560, 1, 0.25, 0.7f); part (1.51, 0.7, 1); v->coef[0] = t60 (0.22, sr); nz (2500, 2, 0.01, 0.1f); break;   // cloche à vache jouet
                case 8: tone (330, 330, 0.45, 0.55, 0.9f); break;                                                    // ressort « boing »
                case 9: tone (115, 46, 0.06, 0.3, 1.f); nz (1500, 1, 0.008, 0.2f); break;                           // grosse caisse jouet
                case 10: nz (7200, 0.7, 0.14, 1.6f); v->toneEnv = 0; break;                                          // hochet
                case 11: default: tone (1150, 2150, 0.07, 0.2, 0.55f); break;                                       // couinement
            }
            for (int k = 0; k < v->np; ++k) v->env[k] = 1;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
void WindupEngine::renderVoice (Voice& v, float& l, float& r, double pitchMul)
{
    v.t += invSr;
    float s = 0;
    if (v.inst == Inst::Drum)
    {
        const int d = v.drum;
        if (v.clicks > 0) { v.clickT += invSr; if (v.clickT >= v.clickGap || v.t <= invSr * 1.5) { v.noiseEnv = 1; --v.clicks; v.clickT = 0; } }
        v.pitchEnv *= v.pitchCoef;
        double f;
        if (d == 8) f = v.f0 * (1.0 + 0.3 * std::sin (twoPi * 17.0 * v.t) * v.pitchEnv);
        else if (d == 11) f = v.f1hz + (v.f0 - v.f1hz) * v.pitchEnv;
        else f = v.f1hz + (v.f0 - v.f1hz) * v.pitchEnv;
        f *= pitchMul;
        if (v.toneEnv > 1e-5f)
        {
            v.dph = wrap (v.dph + f * invSr);
            float t = fastSin (v.dph);
            for (int k = 0; k < v.np; ++k) { v.ph[k] = wrap (v.ph[k] + f * v.ratio[k] * invSr); v.env[k] *= v.coef[k]; t += v.amp[k] * v.env[k] * fastSin (v.ph[k]); }
            if (d == 7) t = std::tanh (t * 2.5f) * 0.6f;
            s += t * v.toneEnv * v.toneAmp; v.toneEnv *= v.toneCoef;
        }
        if (v.noiseEnv > 1e-5f)
        {
            float n = v.nf.bp (noise()) * v.noiseEnv * v.noiseAmp * 2.2f;
            if (d == 10 && v.t < 0.02) n *= (float) (v.t / 0.02);
            s += n; v.noiseEnv *= v.noiseCoef;
        }
        s *= 0.9f * (0.35f + 0.65f * v.vel);
        if (v.toneEnv < 1e-4f && v.noiseEnv < 1e-4f && v.clicks == 0 && v.t > 0.04) v.active = false;
    }
    else
    {
        switch (v.stage)
        {
            case 0: v.aenv += aInc; if (v.aenv >= 1) { v.aenv = 1; v.stage = 1; } break;
            case 1: if (v.inst == Inst::Fanfare) { v.aenv = prm.sustain + (v.aenv - prm.sustain) * dCoef; if (std::abs (v.aenv - prm.sustain) < 1e-4f) v.stage = 2; } break;
            case 2: break;
            default: v.aenv *= rCoef; break;
        }
        const double hz = v.hz * pitchMul;
        if (v.inst == Inst::Fanfare)
        {
            // trompette jouet : impulsion étroite, deux formants nasillards, vibrato qui arrive doucement
            const double vib = 1.0 + 0.009 * std::min (1.0, v.t / 0.35) * fastSin (wrap (v.t * 5.6));
            const double dt = hz * vib * invSr;
            v.ph[0] = wrap (v.ph[0] + dt);
            double t2 = v.ph[0] + 0.72; t2 -= std::floor (t2);
            const double pulse = (v.ph[0] < 0.28 ? 1.0 : -1.0) + polyBlep (v.ph[0], dt) - polyBlep (t2, dt);
            const float x = v.lp.lp ((float) pulse);
            s = (v.f1.bp (x) * 1.1f + v.f2.bp (x) * 0.7f + x * 0.25f) * 0.55f * v.aenv * (0.5f + 0.5f * v.vel);
            if (v.stage == 3 && v.aenv < 1e-4f) v.active = false;
        }
        else
        {
            float sum = 0, mx = 0;
            for (int k = 0; k < v.np; ++k)
            {
                v.ph[k] = wrap (v.ph[k] + hz * v.ratio[k] * invSr); v.env[k] *= v.coef[k];
                const float e = v.amp[k] * v.env[k]; sum += e * fastSin (v.ph[k]); mx = std::max (mx, e);
            }
            if (v.t < 0.002) sum *= (float) (v.t / 0.002);            // pas de clic numérique
            s = sum * 0.42f * v.aenv * (0.35f + 0.65f * v.vel);
            if (mx * v.aenv < 1e-4f) v.active = false;
        }
    }
    const float pan = std::clamp (v.pan, -1.f, 1.f);
    l += s * std::sqrt (0.5f * (1 - pan)) * 1.41f;
    r += s * std::sqrt (0.5f * (1 + pan)) * 1.41f;
}

// ---------------------------------------------------------------------------
void WindupEngine::render (float* outL, float* outR, int n)
{
    aInc = (float) (1.0 / (std::max (0.3f, prm.attackMs) * 0.001 * sr));
    dCoef = t60 (prm.decayMs * 0.001, sr); rCoef = t60 (prm.releaseMs * 0.001, sr);
    const float smA = onePole (15, sr), upA = onePole (40, sr), downA = onePole (2.5, sr);
    const float levelT = std::pow (10.f, prm.levelDb / 20.f);
    const double tuneMul = 1.0;
    const int dlSz = (int) dlL.size();
    const double spb = 60.0 / bpm;
    const double dlTarget = std::clamp (0.75 * spb, 0.05, 1.4) * sr;       // croche pointée
    float peak = 0;
    const bool playing = activeVoices() > 0 || heldCount > 0;

    // ressort : se détend pendant le jeu, se remonte avec la clé (cliquet)
    if (windTarget > 0)
    {
        tens = std::min (1.f, tens + (float) (n * invSr / 0.7));
        if (tens >= 1.f && windClicks <= 0) windTarget = -1;
    }
    else if (prm.springOn && playing) tens = std::max (0.f, tens - (float) (n * invSr / prm.springTime));
    const float slack = prm.springOn ? 1.f - tens : 0.f;
    const double droopCents = -70.0 * std::pow ((double) slack, 1.5);

    for (int i = 0; i < n; ++i)
    {
        float l = 0, r = 0;
        // cliquet du remontage
        if (windClicks > 0)
        {
            windNext -= invSr;
            if (windNext <= 0) { addClick (2900 + 300 * noise(), 5200, 0.5f, 4.f, 0.25f); addClick (900, 1700, 0.18f, 9.f, 0.25f); --windClicks; windNext = 0.055; }
        }
        // tic-tac au tempo (calé sur l'hôte quand il joue, sinon libre tant que des notes sonnent)
        if (prm.tickOn)
        {
            double pos = -1;
            if (ppq >= 0) pos = ppq + i * invSr / spb;
            else if (playing) { freeBeats += invSr / spb * (prm.springOn ? 0.55 + 0.45 * tens : 1.0); pos = freeBeats; }
            if (pos >= 0)
            {
                const double div = kDiv[prm.tickDiv], q = pos / div; long long s = (long long) std::floor (q); const double f = q - (double) s;
                long long e = s; if ((s & 1) && f < prm.swing * 0.5) e = s - 1;
                if (e != lastStep)
                {
                    lastStep = e;
                    const bool tic = (e & 1) == 0; const float lv = prm.tickLevel * 0.55f;
                    if (tic) addClick (3150, 4800, lv, 5.f, -0.15f); else addClick (2050, 3350, lv * 0.85f, 6.f, 0.15f);
                    fb.ticks.fetch_add (1, std::memory_order_relaxed);
                }
                fb.beat.store ((float) (pos - std::floor (pos)), std::memory_order_relaxed);
            }
        }
        // petits cliquetis aléatoires du mécanisme pendant le jeu
        if (playing && prm.meca > 0.01f)
        {
            rattleAcc += invSr * prm.meca * 4.0;
            if (rattleAcc >= 1) { rattleAcc -= 1 + 0.8 * std::abs (noise()); addClick (2200 + 3800 * std::abs (noise()), 6500, 0.05f * prm.meca, 2.f, noise() * 0.6f); }
        }
        // pleurage : lent + scintillement ; le ressort détendu fait baisser et trembler la hauteur
        if ((i & 15) == 0)
        {
            wobPh1 = wrap (wobPh1 + 16 * invSr * 0.73); wobPh2 = wrap (wobPh2 + 16 * invSr * 6.1);
        }
        const double wob = (prm.wobble + slack * 0.35) * (11.0 * fastSin (wobPh1) + 3.5 * fastSin (wobPh2));
        const double pitchMul = tuneMul * std::pow (2.0, (wob + droopCents) / 1200.0);
        for (auto& v : voices) if (v.active) renderVoice (v, l, r, pitchMul);
        l *= 0.5f; r *= 0.5f;
        for (auto& c : clicks)
        {
            if (! c.on) continue;
            if (c.delay > 0) { --c.delay; continue; }
            const float x = noise() * c.env; c.env *= c.coef;
            const float o = (c.a.bp (x) * 1.0f + c.b.bp (x) * 0.7f) * c.amp * 1.6f;
            l += o * (0.5f - 0.5f * c.pan) * 1.2f; r += o * (0.5f + 0.5f * c.pan) * 1.2f;
            if (c.env < 1e-4f && std::abs (c.a.ic1) + std::abs (c.b.ic1) < 1e-5f) c.on = false;
            else if (c.env < 1e-6f) c.on = false;
        }

        // Tone
        toneSm += smA * (prm.tone - toneSm);
        if ((i & 15) == 0) toneA = onePole (600.0 * std::pow (2.0, toneSm * 5.0), sr);
        toneL1 += toneA * (l - toneL1); toneL2 += toneA * (toneL1 - toneL2);
        toneR1 += toneA * (r - toneR1); toneR2 += toneA * (toneR1 - toneR2);
        l = toneL2; r = toneR2;

        const float dry = 0.5f * (std::abs (l) + std::abs (r));
        follower += (dry > follower ? upA : downA) * (dry - follower);

        // Écho ping-pong au tempo
        echoSm += smA * (prm.echo - echoSm);
        dlTimeSm += (dlTimeSm <= 0 ? 1.f : 0.0005f) * ((float) dlTarget - dlTimeSm);
        if (echoSm > 0.001f)
        {
            double rp = dlW - dlTimeSm; while (rp < 0) rp += dlSz;
            const int i0 = (int) rp, i1 = (i0 + 1) % dlSz; const float fr = (float) (rp - i0);
            const float yl = dlL[(size_t) i0] + fr * (dlL[(size_t) i1] - dlL[(size_t) i0]), yr = dlR[(size_t) i0] + fr * (dlR[(size_t) i1] - dlR[(size_t) i0]);
            const float fbk = 0.22f + 0.48f * echoSm;
            dlL[(size_t) dlW] = (l + r) * 0.5f + yr * fbk; dlR[(size_t) dlW] = yl * fbk;
            dlW = (dlW + 1) % dlSz;
            l += yl * echoSm * 0.75f; r += yr * echoSm * 0.75f;
        }
        else { dlL[(size_t) dlW] = 0; dlR[(size_t) dlW] = 0; dlW = (dlW + 1) % dlSz; }

        // Espace
        spaceSm += smA * (prm.space - spaceSm);
        if (spaceSm > 0.001f)
        {
            const float in = (l + r) * 0.5f * 0.03f, fbk = 0.76f + 0.17f * spaceSm, damp = 0.32f;
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
            const float wet = spaceSm * 1.3f;
            l = l * (1 - 0.25f * spaceSm) + rl * wet; r = r * (1 - 0.25f * spaceSm) + rr * wet;
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
    if (windTarget > 0 && tens >= 1.f && windClicks <= 0) windTarget = -1;
    fb.energy.store (std::min (1.f, follower * 3.f), std::memory_order_relaxed);
    fb.held.store (heldCount, std::memory_order_relaxed);
    fb.voices.store (activeVoices(), std::memory_order_relaxed);
    fb.tension.store (tens, std::memory_order_relaxed);
    fb.level.store (peak, std::memory_order_relaxed);
}
} // namespace wd
