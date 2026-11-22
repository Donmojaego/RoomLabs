#include "SynthEngine.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <mutex>

namespace kal
{
namespace
{
constexpr double twoPi = 6.283185307179586, pi = 3.141592653589793;
inline float t60 (double s, double sr) { return (float) std::exp (std::log (0.001) / (std::max (s, 0.0005) * sr)); }
inline float onePole (double hz, double sr) { return (float) (1.0 - std::exp (-twoPi * std::min (hz, sr * 0.45) / sr)); }
inline double wrap (double p) { return p - std::floor (p); }

// FFT complexe (radix 2) pour construire les tables limitées en bande
void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = twoPi / (double) len * (inverse ? 1 : -1); const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1); for (size_t j = 0; j < len / 2; ++j) { auto u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } }
    }
}
double besselJ (int k, double x)
{
    double sum = 0, term = 1; const double hx = x / 2;
    for (int i = 1; i <= k; ++i) term *= hx / i;
    for (int m = 0; m < 30; ++m) { sum += (m % 2 ? -term : term); term *= hx * hx / ((m + 1.0) * (m + 1.0 + k)); }
    return sum;
}
// amplitude de l'harmonique h (1..512) pour la table t à la position x (0..1)
double harmonic (int t, int h, double x)
{
    switch (t)
    {
        case 0: {   // Analogique : sinus -> triangle -> dent de scie -> carré
            const double saw = 1.0 / h, sq = (h % 2) ? 1.0 / h : 0, tri = (h % 2) ? ((h / 2) % 2 ? -1.0 : 1.0) / (h * h) : 0, sine = h == 1 ? 1 : 0;
            const double s = x * 3; if (s < 1) return sine * (1 - s) + tri * s * 1.2; if (s < 2) return tri * 1.2 * (2 - s) + saw * (s - 1); return saw * (3 - s) + sq * (s - 2); }
        case 1: { const double w = 0.5 - 0.45 * x; return std::sin (pi * h * w) / h * 1.6; }                  // Impulsion (PWM)
        case 2: {   // Formants : a -> é -> i -> o -> ou (fondamentale 220 Hz)
            static const double F[5][2] = { { 800, 1200 }, { 450, 1800 }, { 300, 2300 }, { 500, 900 }, { 320, 800 } };
            const double s = x * 4; const int i = std::min (3, (int) s); const double fr = s - i;
            const double f1 = F[i][0] * (1 - fr) + F[i + 1][0] * fr, f2 = F[i][1] * (1 - fr) + F[i + 1][1] * fr, hz = 220.0 * h;
            return (1.0 / std::sqrt ((double) h)) * (std::exp (-std::pow ((hz - f1) / 160, 2)) + 0.7 * std::exp (-std::pow ((hz - f2) / 260, 2)) + 0.04); }
        case 3: {   // Verre : partiels impairs clairsemés, brillance qui monte
            const bool keep = h == 1 || h == 3 || h == 7 || h == 11 || h == 17 || h == 23 || (x > 0.5 && h % 4 == 1) || (x > 0.8 && h % 2 == 0 && h < 40);
            return keep ? std::pow (h, -0.55 - 0.5 * (1 - x)) : 0; }
        case 4: {   // Orgue : registres de tirettes
            static const int reg[5][9] = { { 8, 8, 6, 0, 0, 0, 0, 0, 0 }, { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, { 8, 6, 8, 6, 0, 4, 0, 0, 0 }, { 6, 8, 8, 8, 6, 4, 2, 2, 0 }, { 8, 8, 8, 8, 8, 8, 8, 8, 8 } };
            static const int hh[9] = { 1, 2, 3, 4, 6, 8, 10, 12, 16 };
            const double s = x * 4; const int i = std::min (3, (int) s); const double fr = s - i;
            for (int k = 0; k < 9; ++k) if (hh[k] == h) return (reg[i][k] * (1 - fr) + reg[i + 1][k] * fr) / 8.0 * 0.6;
            return 0; }
        case 5: {   // Numérique : spectre de FM (porteuse 1, modulante 2), index 0 -> 7
            const double I = x * 7; if (h % 2 == 0) return 0; const int k = (h - 1) / 2; return std::abs (besselJ (k, I)) + (h == 1 ? 0 : 0); }
        case 6: {   // Chœur : dent de scie douce + formant « a » / « o »
            const double hz = 196.0 * h, f1 = 700 - 250 * x, f2 = 1150 - 300 * x;
            return std::pow (h, -1.2) * (0.35 + std::exp (-std::pow ((hz - f1) / 220, 2)) + 0.6 * std::exp (-std::pow ((hz - f2) / 300, 2))); }
        default: {  // Spectral : harmoniques pseudo-aléatoires qui se transforment
            auto rnd = [] (int a, int b) { uint32_t z = (uint32_t) (a * 374761393u + b * 668265263u); z = (z ^ (z >> 13)) * 1274126177u; return (double) (z & 0xFFFF) / 65535.0; };
            const double s = x * 3; const int i = std::min (2, (int) s); const double fr = s - i;
            return (rnd (h, i) * (1 - fr) + rnd (h, i + 1) * fr) * std::pow (h, -0.9) * (h < 64 ? 1 : 0.2); }
    }
}
} // namespace

const char* WaveTables::name (int t) { static const char* n[] = { "Analogique", "Impulsion", "Formants", "Verre", "Orgue", "Numérique", "Chœur", "Spectral" }; return n[std::clamp (t, 0, 7)]; }

const WaveTables& WaveTables::get() { static const WaveTables w; return w; }

WaveTables::WaveTables() : data ((size_t) kTables * kFrames * kMips * kSize, 0.f)
{
    std::vector<std::complex<double>> spec (kSize);
    for (int t = 0; t < kTables; ++t)
        for (int f = 0; f < kFrames; ++f)
        {
            const double x = f / (double) (kFrames - 1);
            std::vector<double> amp (513, 0.0); double norm0 = 1;
            for (int h = 1; h <= 512; ++h) amp[(size_t) h] = harmonic (t, h, x);
            for (int m = 0; m < kMips; ++m)
            {
                const int maxH = 512 >> m;
                std::fill (spec.begin(), spec.end(), std::complex<double> (0, 0));
                for (int h = 1; h <= maxH; ++h) { const double a = amp[(size_t) h]; spec[(size_t) h] = std::complex<double> (0, -a / 2); spec[(size_t) (kSize - h)] = std::complex<double> (0, a / 2); }
                fft (spec, true);
                double peak = 1e-9; for (auto& c : spec) peak = std::max (peak, std::abs (c.real()));
                float* dst = data.data() + (((size_t) t * kFrames + (size_t) f) * kMips + (size_t) m) * kSize;
                // même normalisation pour tous les mips d'une image (pas de saut de volume entre octaves)
                if (m == 0) norm0 = peak;
                for (int i = 0; i < kSize; ++i) dst[i] = (float) (spec[(size_t) i].real() / norm0 * 0.9);
            }
        }
}

// ---------------------------------------------------------------------------
void SynthEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 1000 ? sampleRate : 48000; invSr = 1.0 / sr;
    WaveTables::get();
    chL.assign ((size_t) (sr * 0.05) + 8, 0.f); chR.assign (chL.size(), 0.f);
    dlL.assign ((size_t) (sr * 2.1) + 8, 0.f); dlR.assign (dlL.size(), 0.f);
    static const int fdnLen[8] = { 1433, 1601, 1867, 2053, 2251, 2399, 2617, 2797 };
    for (int i = 0; i < 8; ++i) fdn.d[(size_t) i].assign ((size_t) (fdnLen[i] * sr / 44100.0 * 1.6) + 64, 0.f);
    grainSrc.assign ((size_t) 16384, 0.f); grainSrcTable = -1;
    reset();
}

void SynthEngine::reset()
{
    for (auto& v : voices) v = Voice {};
    heldCount = 0;
    for (auto* b : { &chL, &chR, &dlL, &dlR }) std::fill (b->begin(), b->end(), 0.f);
    for (auto& d : fdn.d) std::fill (d.begin(), d.end(), 0.f);
    fdn.w.fill (0); fdn.lp.fill (0); chW = dlW = 0; follow = lowF = highF = 0; limGain = 1; tiltLo = tiltLoR = 0; dlTimeSm = 0;
}

int SynthEngine::activeVoices() const { int n = 0; for (auto& v : voices) n += v.active; return n; }

void SynthEngine::rebuildGrainSource()
{
    // source granulaire sans échantillon : la table A rendue à do 4, à sa position courante
    const auto& W = WaveTables::get(); const double pos = std::clamp ((double) prm.posA, 0.0, 1.0) * (WaveTables::kFrames - 1);
    const int f0 = (int) pos, f1 = std::min (f0 + 1, WaveTables::kFrames - 1); const float fr = (float) (pos - f0);
    const float* a = W.frame (prm.tableA, f0, 2); const float* b = W.frame (prm.tableA, f1, 2);
    double ph = 0; const double inc = grainSrcHz * WaveTables::kSize / sr;
    for (size_t i = 0; i < grainSrc.size(); ++i)
    {
        const int i0 = (int) ph; const float x = (float) (ph - i0); const int i1 = (i0 + 1) & (WaveTables::kSize - 1);
        const float va = a[i0] + x * (a[i1] - a[i0]), vb = b[i0] + x * (b[i1] - b[i0]);
        grainSrc[i] = va + fr * (vb - va);
        ph += inc; if (ph >= WaveTables::kSize) ph -= WaveTables::kSize;
    }
    grainSrcTable = prm.tableA; grainSrcPos = prm.posA;
}

void SynthEngine::noteOn (int note, float vel)
{
    vel = std::clamp (vel, 0.f, 1.f);
    fb.noteHits.fetch_add (1, std::memory_order_relaxed); fb.lastNote.store (note, std::memory_order_relaxed); fb.lastVel.store (vel, std::memory_order_relaxed);
    int w = 0; for (int i = 0; i < heldCount; ++i) if (heldStack[(size_t) i] != note) heldStack[(size_t) w++] = heldStack[(size_t) i];
    heldCount = std::min (w, 15); heldStack[(size_t) heldCount++] = note;
    if (prm.mono)
    {
        Voice& v = voices[0]; const bool legato = v.active && ! v.released;
        startVoice (v, note, vel, legato); return;
    }
    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.note == note) { v = &x; break; }       // même note : on la relance
    if (! v) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (! v) { float best = 2; for (auto& x : voices) if (x.released && x.aEnv < best) { best = x.aEnv; v = &x; } }
    if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    startVoice (*v, note, vel, false);
}

void SynthEngine::noteOff (int note)
{
    int w = 0; for (int i = 0; i < heldCount; ++i) if (heldStack[(size_t) i] != note) heldStack[(size_t) w++] = heldStack[(size_t) i];
    heldCount = w;
    if (prm.mono)
    {
        Voice& v = voices[0];
        if (v.active && v.note == note) { if (heldCount > 0) startVoice (v, heldStack[(size_t) heldCount - 1], v.vel, true); else { v.released = true; v.aStage = 3; v.fStage = 3; } }
        return;
    }
    for (auto& v : voices) if (v.active && ! v.released && v.note == note) { v.released = true; v.aStage = 3; v.fStage = 3; }
}

void SynthEngine::allNotesOff() { heldCount = 0; for (auto& v : voices) if (v.active) { v.released = true; v.aStage = 3; v.fStage = 3; } }

void SynthEngine::startVoice (Voice& v, int note, float vel, bool legato)
{
    const double hz = 440.0 * std::pow (2.0, (note - 69.0) / 12.0);
    if (legato) { v.ctl = 0; v.note = note; v.hzTarget = hz; v.vel = std::max (v.vel, vel); v.released = false; if (v.aStage == 3) { v.aStage = 1; v.fStage = 1; } return; }
    const bool wasActive = v.active; const float keep = wasActive ? v.aEnv : 0.f;
    v.active = true; v.released = false; v.note = note; v.vel = vel; v.age = ++ageCounter;
    v.hzTarget = hz; v.hz = (prm.glideMs > 0.5f && wasActive) ? v.hz : hz;
    v.aEnv = keep; v.aStage = 0; v.fEnv = 0; v.fStage = 0;
    for (size_t u = 0; u < 4; ++u) { v.phA[u] = wrap (0.17 * (double) u + 0.31 * noise()) * WaveTables::kSize; v.phB[u] = wrap (0.23 * (double) u + 0.29 * noise()) * WaveTables::kSize; }
    v.phSub = 0; v.lfoPh = 0; v.ctl = 0;
    if (! wasActive) { v.f1L.reset(); v.f2L.reset(); v.f1R.reset(); v.f2R.reset(); }
    // échantillons : région selon la note et la vélocité
    v.reg = nullptr; v.sDone = true; patch = patchPtr.load (std::memory_order_acquire);
    if (prm.smpOn && patch)
    {
        const int vv = (int) std::lround (vel * 127);
        const SampleRegion* best = nullptr; int bestD = 1 << 20;
        for (auto& r : patch->regions)
        {
            const int dn = note < r.lo ? r.lo - note : (note > r.hi ? note - r.hi : 0), dv = vv < r.vlo ? r.vlo - vv : (vv > r.vhi ? vv - r.vhi : 0);
            const int d = dn * 200 + dv; if (d < bestD && r.frames > 0) { bestD = d; best = &r; }
        }
        if (best) { v.reg = best; v.sPos = 0; v.sDone = false; v.sInc = std::pow (2.0, (note - best->root + best->tune + prm.smpTune) / 12.0) * best->sr / sr; }
    }
    for (auto& g : v.grains) g.on = false;
    v.grainAcc = 0;
}

// ---------------------------------------------------------------------------
void SynthEngine::renderVoice (Voice& v, float& outL, float& outR, const float*)
{
    const auto& W = WaveTables::get();
    // enveloppes (coefficients calculés une fois par bloc)
    auto env = [] (float& e, int& st, const EnvC& c, float sus) {
        switch (st)
        {
            case 0: e += c.aInc; if (e >= 1) { e = 1; st = 1; } break;
            case 1: e = sus + (e - sus) * c.dC; if (std::abs (e - sus) < 1e-4f) st = 2; break;
            case 2: e = sus; break;
            default: e *= c.rC; break;
        }
    };
    env (v.aEnv, v.aStage, envA, prm.aS);
    env (v.fEnv, v.fStage, envF, prm.fS);
    // glisse
    if (v.hz != v.hzTarget) { const double g = prm.glideMs > 0.5f ? 1.0 - std::exp (-1.0 / (prm.glideMs * 0.001 * sr)) : 1.0; v.hz += (v.hzTarget - v.hz) * g; if (std::abs (v.hz - v.hzTarget) < 1e-4) v.hz = v.hzTarget; }
    v.lfoPh = wrap (v.lfoPh + prm.lfoRate * invSr);
    const float lfo = (float) std::sin (twoPi * v.lfoPh);
    if (v.ctl-- <= 0)
    {
        v.ctl = 15;
        const double hzc = v.hz * std::exp2 ((bend * prm.bendRange + prm.lfoVib * 0.5f * lfo) / 12.0);
        v.hzNow = hzc;
        const double hzB = hzc * std::exp2 ((prm.semiB + prm.fineB * 0.01) / 12.0);
        for (int u = 0; u < 4; ++u) { v.incA[u] = hzc * detRatio[u] * WaveTables::kSize * invSr; v.incB[u] = hzB * detRatio[u] * WaveTables::kSize * invSr; }
        auto mipOf = [&] (double f) { return std::clamp ((int) std::ceil (std::log2 (std::max (1.0, 512.0 * f / (sr * 0.45)))), 0, WaveTables::kMips - 1); };
        v.mip = mipOf (hzc); v.mipB = mipOf (hzB);
        double fc = 30.0 * std::exp2 (prm.cutoff * 9.7 + (v.note - 60) / 12.0 * prm.keytrack + (prm.fEnv * v.fEnv + prm.lfoCut * 0.5f * lfo) * 6.0);
        fc = std::clamp (fc, 20.0, sr * 0.46);
        const double g = std::tan (pi * fc / sr); v.fk = 2.0 - 1.95 * std::clamp ((double) prm.reso, 0.0, 1.0);
        v.fa1 = 1.0 / (1.0 + g * (g + v.fk)); v.fa2 = g * v.fa1; v.fa3 = g * v.fa2;
    }
    const double hz = v.hzNow;
    float l = 0, r = 0;

    if (prm.wtOn)
    {
        const int uni = std::clamp (prm.unison, 1, 4);
        auto osc = [&] (int table, float pos, int mip, const double* inc, std::array<double, 4>& ph, float gain) {
            const float p = std::clamp (pos + prm.wtEnv * v.fEnv + prm.wtLfo * 0.5f * lfo, 0.f, 1.f) * (WaveTables::kFrames - 1);
            const int f0 = (int) p, f1 = std::min (f0 + 1, WaveTables::kFrames - 1); const float fr = p - (float) f0;
            const float* A = W.frame (table, f0, mip); const float* B = W.frame (table, f1, mip);
            const float ug = gain * uniNorm;
            for (int u = 0; u < uni; ++u)
            {
                double& x = ph[(size_t) u]; x += inc[u]; if (x >= WaveTables::kSize) x -= WaveTables::kSize;
                const int i0 = (int) x; const float t = (float) (x - i0); const int i1 = (i0 + 1) & (WaveTables::kSize - 1);
                const float a = A[i0] + t * (A[i1] - A[i0]), b = B[i0] + t * (B[i1] - B[i0]);
                const float s = (a + fr * (b - a)) * ug;
                l += s * panL[u]; r += s * panR[u];
            }
        };
        osc (prm.tableA, prm.posA, v.mip, v.incA, v.phA, 1.f - prm.mixB);
        if (prm.mixB > 0.001f) osc (prm.tableB, prm.posB, v.mipB, v.incB, v.phB, prm.mixB);
        if (prm.sub > 0.001f) { v.phSub = wrap (v.phSub + hz * 0.5 * invSr); const float s = (float) std::sin (twoPi * v.phSub) * prm.sub; l += s; r += s; }
        if (prm.noise > 0.001f) { l += noise() * prm.noise * 0.3f; r += noise() * prm.noise * 0.3f; }
    }
    // granulaire : nuage de grains (source = échantillon de la voix, sinon la table A rendue)
    if (prm.granOn)
    {
        const float* src = nullptr; int srcLen = 0; double srcHz = grainSrcHz; int stride = 1;
        if (v.reg && prm.smpOn) { src = v.reg->data.data(); srcLen = v.reg->frames; srcHz = 440.0 * std::pow (2.0, (v.reg->root - 69.0) / 12.0) * (sr / v.reg->sr); stride = 2; }
        else { src = grainSrc.data(); srcLen = (int) grainSrc.size(); }
        if (src && srcLen > 64)
        {
            v.grainAcc += prm.granDensity * invSr;
            while (v.grainAcc >= 1)
            {
                v.grainAcc -= 1;
                for (auto& g : v.grains) if (! g.on)
                {
                    g.on = true; g.age = 0; g.len = std::max (64, (int) (prm.granSize * 0.001 * sr));
                    const double centre = prm.granPos * (srcLen - g.len - 2);
                    g.pos = std::clamp (centre + (noise() * 0.5 + 0.5 - 0.5) * prm.granSpray * srcLen, 0.0, (double) std::max (0, srcLen - 2));
                    g.inc = hz / srcHz * std::pow (2.0, (noise() * prm.granPitch * 12.0) / 12.0);
                    g.pan = noise() * 0.8f; g.amp = 0.7f + 0.3f * noise();
                    break;
                }
            }
            float gl = 0, gr = 0;
            for (auto& g : v.grains)
            {
                if (! g.on) continue;
                const int i0 = (int) g.pos; if (i0 + 1 >= srcLen) { g.on = false; continue; }
                const float t = (float) (g.pos - i0);
                const float s = stride == 2 ? 0.5f * (src[i0 * 2] + src[i0 * 2 + 1] + t * (src[i0 * 2 + 2] + src[i0 * 2 + 3] - src[i0 * 2] - src[i0 * 2 + 1])) : src[i0] + t * (src[i0 + 1] - src[i0]);
                const float w = 0.5f - 0.5f * (float) std::cos (twoPi * g.age / g.len);
                gl += s * w * g.amp * (0.5f - 0.5f * g.pan); gr += s * w * g.amp * (0.5f + 0.5f * g.pan);
                g.pos += g.inc; if (++g.age >= g.len) g.on = false;
            }
            const float gm = prm.granMix * 0.55f;
            l = l * (1 - prm.granMix * 0.6f) + gl * gm; r = r * (1 - prm.granMix * 0.6f) + gr * gm;
        }
    }
    // échantillons
    if (prm.smpOn && v.reg && ! v.sDone)
    {
        const auto& R = *v.reg; const int i0 = (int) v.sPos;
        if (i0 + 2 < R.frames)
        {
            const float t = (float) (v.sPos - i0); const float* d = R.data.data();
            auto cub = [&] (int ch) {   // Hermite 4 points
                const int im1 = std::max (0, i0 - 1);
                const float y0 = d[im1 * 2 + ch], y1 = d[i0 * 2 + ch], y2 = d[(i0 + 1) * 2 + ch], y3 = d[(i0 + 2) * 2 + ch];
                const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2 * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * t + c2) * t + c1) * t + y1; };
            const float g = R.gain * prm.smpLevel * 1.1f;
            l += cub (0) * g; r += cub (1) * g;
            v.sPos += v.sInc;
            if (R.loop && R.loopEnd > R.loopStart + 16 && v.sPos >= R.loopEnd) v.sPos -= (R.loopEnd - R.loopStart);
        }
        else v.sDone = true;
    }
    // filtre (TPT SVF, saturation douce avant)
    const double k = v.fk, a1 = v.fa1, a2 = v.fa2, a3 = v.fa3;
    auto svf = [&] (Svf& s, float x) {
        const double v3 = x - s.ic2, v1 = a1 * s.ic1 + a2 * v3, v2 = s.ic2 + a2 * s.ic1 + a3 * v3;
        s.ic1 = (float) (2 * v1 - s.ic1); s.ic2 = (float) (2 * v2 - s.ic2);
        return prm.fMode == 0 ? (float) v2 : (prm.fMode == 1 ? (float) (v1 * k) : (float) (x - k * v1 - v2)); };
    float fl = l, fr = r;
    if (prm.drive > 0.01f) { const float drv = 1.f + prm.drive * 4.f, nd = 1.f / std::sqrt (drv); fl = std::tanh (l * drv) * nd * 1.6f; fr = std::tanh (r * drv) * nd * 1.6f; }
    fl = svf (v.f1L, fl); fr = svf (v.f1R, fr);
    if (prm.f24) { fl = svf (v.f2L, fl); fr = svf (v.f2R, fr); }
    const float amp = v.aEnv * (0.35f + 0.65f * v.vel) * (1.f - prm.lfoAmp * 0.5f * (1 - lfo)) * 0.3f;
    outL += fl * amp; outR += fr * amp;
    // fin de voix
    const bool smpOver = ! prm.smpOn || ! v.reg || v.sDone;
    if (v.aStage == 3 && v.aEnv < 1e-4f) v.active = false;
    else if (! prm.wtOn && ! prm.granOn && smpOver && v.aStage != 0) v.active = false;   // échantillon percussif terminé
}

// ---------------------------------------------------------------------------
void SynthEngine::render (float* L, float* R, int n)
{
    patch = patchPtr.load (std::memory_order_acquire);
    if (prm.granOn && (! patch || ! prm.smpOn))
    {
        if (grainCooldown > 0) grainCooldown -= n;
        else if (grainSrcTable != prm.tableA || std::abs (grainSrcPos - prm.posA) > 0.04f) { rebuildGrainSource(); grainCooldown = (int) (sr * 0.1); }
    }
    const float smA = onePole (12, sr), fA = onePole (30, sr), fRel = onePole (3, sr);
    auto mkEnv = [&] (float a, float d, float r) { EnvC c; c.aInc = (float) (invSr / std::max (0.0005, a * 0.001)) * 1.2f; c.dC = t60 (d * 0.001 / 3, sr); c.rC = t60 (r * 0.001 / 3, sr); return c; };
    envA = mkEnv (prm.aA, prm.aD, prm.aR); envF = mkEnv (prm.fA, prm.fD, prm.fR);
    {
        const int uni = std::clamp (prm.unison, 1, 4);
        for (int u = 0; u < 4; ++u) detRatio[u] = uni == 1 ? 1.0 : std::exp2 ((u / (double) (uni - 1) - 0.5) * prm.detune * 0.5 / 12.0);
        uniNorm = 1.f / std::sqrt ((float) uni);
        for (int u = 0; u < 4; ++u) { const float pan = uni == 1 ? 0.f : (float) (u / (double) (uni - 1) - 0.5) * prm.spread; panL[u] = (0.5f - pan * 0.5f) * 1.4f; panR[u] = (0.5f + pan * 0.5f) * 1.4f; }
    }
    const float levelT = std::pow (10.f, prm.levelDb / 20.f);
    const double divBeats[6] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
    const double dlTarget = std::clamp (divBeats[std::clamp (prm.delayDiv, 0, 5)] * 60.0 / tempo * sr, 64.0, (double) dlL.size() - 8);
    const int chSz = (int) chL.size(), dlSz = (int) dlL.size();
    const float tiltA = onePole (700, sr);
    float peak = 0;
    for (int i = 0; i < n; ++i)
    {
        float l = 0, r = 0;
        for (auto& v : voices) if (v.active) renderVoice (v, l, r, nullptr);
        // saturation
        if (prm.fxDrive > 0.001f) { const float d = 1 + prm.fxDrive * 6; l = std::tanh (l * d) / std::tanh (d) * (1 - 0.2f * prm.fxDrive) + l * 0.0f; r = std::tanh (r * d) / std::tanh (d) * (1 - 0.2f * prm.fxDrive); }
        // chorus (deux lignes modulées en quadrature)
        chL[(size_t) chW] = l; chR[(size_t) chW] = r;
        if (prm.chorus > 0.001f)
        {
            chPh = wrap (chPh + 0.6 * invSr);
            auto tap = [&] (const std::vector<float>& b, double ms) { double rp = chW - ms * 0.001 * sr; while (rp < 0) rp += chSz; const int i0 = (int) rp, i1 = (i0 + 1) % chSz; const float t = (float) (rp - i0); return b[(size_t) i0] + t * (b[(size_t) i1] - b[(size_t) i0]); };
            const float cl = tap (chL, 9 + 3.5 * std::sin (twoPi * chPh)), cr = tap (chR, 9 + 3.5 * std::cos (twoPi * chPh));
            l = l * (1 - 0.35f * prm.chorus) + cr * 0.6f * prm.chorus; r = r * (1 - 0.35f * prm.chorus) + cl * 0.6f * prm.chorus;
        }
        chW = (chW + 1) % chSz;
        // écho ping-pong au tempo
        dlTimeSm += (dlTimeSm <= 0 ? 1.f : 0.0008f) * ((float) dlTarget - dlTimeSm);
        {
            double rp = dlW - dlTimeSm; while (rp < 0) rp += dlSz;
            const int i0 = (int) rp, i1 = (i0 + 1) % dlSz; const float t = (float) (rp - i0);
            const float yl = dlL[(size_t) i0] + t * (dlL[(size_t) i1] - dlL[(size_t) i0]), yr = dlR[(size_t) i0] + t * (dlR[(size_t) i1] - dlR[(size_t) i0]);
            const float fbk = std::clamp (prm.delayFb, 0.f, 0.92f);
            dlL[(size_t) dlW] = (prm.delay > 0.001f ? (l + r) * 0.5f : 0.f) + yr * fbk; dlR[(size_t) dlW] = yl * fbk;
            dlW = (dlW + 1) % dlSz;
            l += yl * prm.delay; r += yr * prm.delay;
        }
        // réverbération FDN 8 lignes (Hadamard), amortie, légèrement modulée
        if (prm.reverb > 0.001f)
        {
            fdn.modPh = wrap (fdn.modPh + 0.3 * invSr);
            const float fbk = 0.72f + 0.26f * std::clamp (prm.revSize, 0.f, 1.f), damp = 0.25f + 0.35f * (1 - prm.revSize);
            float o[8];
            for (int k = 0; k < 8; ++k)
            {
                auto& d = fdn.d[(size_t) k]; const int sz = (int) d.size();
                const double len = sz - 40 - (k == 0 ? 12 * (1 + std::sin (twoPi * fdn.modPh)) : 0);
                double rp = fdn.w[(size_t) k] - len; while (rp < 0) rp += sz;
                const int i0 = (int) rp, i1 = (i0 + 1) % sz; const float t = (float) (rp - i0);
                float y = d[(size_t) i0] + t * (d[(size_t) i1] - d[(size_t) i0]);
                fdn.lp[(size_t) k] += (1 - damp) * (y - fdn.lp[(size_t) k]); o[k] = fdn.lp[(size_t) k];
            }
            // Hadamard 8 (normalisée)
            float h[8]; for (int k = 0; k < 8; ++k) h[k] = o[k];
            for (int len = 1; len < 8; len <<= 1) for (int a = 0; a < 8; a += 2 * len) for (int b = a; b < a + len; ++b) { const float x = h[b], y = h[b + len]; h[b] = x + y; h[b + len] = x - y; }
            const float in = (l + r) * 0.5f;
            for (int k = 0; k < 8; ++k)
            {
                auto& d = fdn.d[(size_t) k];
                d[(size_t) fdn.w[(size_t) k]] = in * (k % 2 ? 0.35f : -0.35f) + h[k] * 0.353553f * fbk;
                if (++fdn.w[(size_t) k] >= (int) d.size()) fdn.w[(size_t) k] = 0;
            }
            const float rl = o[0] + o[2] + o[4] + o[6], rr = o[1] + o[3] + o[5] + o[7];
            l += rl * prm.reverb * 0.55f; r += rr * prm.reverb * 0.55f;
        }
        // largeur et tilt
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * std::clamp (prm.width, 0.f, 1.5f);
        l = m + s; r = m - s;
        if (std::abs (prm.tilt) > 0.01f)
        {
            tiltLo += tiltA * (l - tiltLo); tiltLoR += tiltA * (r - tiltLoR);
            const float gLo = 1 - prm.tilt * 0.5f, gHi = 1 + prm.tilt * 0.5f;
            l = tiltLo * gLo + (l - tiltLo) * gHi; r = tiltLoR * gLo + (r - tiltLoR) * gHi;
        }
        levelSm += smA * (levelT - levelSm); l *= levelSm; r *= levelSm;
        // limiteur doux (gain lissé + écrêtage arrondi au-delà de -1 dBFS)
        const float a = std::max (std::abs (l), std::abs (r));
        const float want = a * limGain > 0.89f ? 0.89f / std::max (a, 1e-9f) : 1.f;
        limGain += (want < limGain ? 0.5f : 0.0004f) * (want - limGain);
        l *= limGain; r *= limGain;
        auto soft = [] (float x) { const float ax = std::abs (x); return ax < 0.89f ? x : std::copysign (0.89f + 0.1f * std::tanh ((ax - 0.89f) / 0.1f), x); };
        l = soft (l); r = soft (r);
        if (! std::isfinite (l)) l = 0;
        if (! std::isfinite (r)) r = 0;
        L[i] = l; R[i] = r;
        const float e = 0.5f * (std::abs (l) + std::abs (r));
        follow += (e > follow ? fA : fRel) * (e - follow);
        lowF += 0.002f * (std::abs (m) - lowF); highF += 0.02f * (std::abs (s) - highF);
        peak = std::max (peak, a);
    }
    fb.energy.store (std::min (1.f, follow * 3.f), std::memory_order_relaxed);
    fb.voices.store (activeVoices(), std::memory_order_relaxed);
    fb.level.store (peak, std::memory_order_relaxed);
}
} // namespace kal
