// Tests du générateur Noise : niveaux, pentes spectrales, stabilité, mono/stéréo, démarrage muet, absence de clic.
#define _USE_MATH_DEFINES
#include "../plugins/noise/NoiseDSP.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <string>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace rl::nz;
static std::string rep = "\n## Noise — générateur (captures de 60 s)\n\n"; static int fails = 0;
static void line (bool ok, const std::string& s) { std::printf ("%s  %s\n", ok ? "OK " : "ÉCHEC", s.c_str()); rep += std::string ("- [") + (ok ? "OK" : "ÉCHEC") + "] " + s + "\n"; if (! ok) ++fails; }
template <typename... A> static std::string F (const char* f, A... a) { char b[512]; std::snprintf (b, sizeof b, f, a...); return b; }

static void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1) { const double ang = -2 * M_PI / len; const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1); for (size_t j = 0; j < len / 2; ++j) { auto u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } } }
}
// puissance moyenne par octave (Welch, fenêtre de Hann, 8192 points) : centres 125 Hz .. 8 kHz
static std::vector<double> octaves (const std::vector<float>& x, double sr)
{
    const size_t N = 8192; std::vector<double> P (N / 2 + 1, 0); int segs = 0;
    for (size_t o = 0; o + N <= x.size(); o += N / 2, ++segs)
    { std::vector<std::complex<double>> a (N); for (size_t i = 0; i < N; ++i) a[i] = x[o + i] * (0.5 - 0.5 * std::cos (2 * M_PI * i / N)); fft (a); for (size_t k = 0; k <= N / 2; ++k) P[k] += std::norm (a[k]); }
    std::vector<double> out;
    for (double fc = 125; fc <= 8000; fc *= 2)
    { double s = 0; for (size_t k = 1; k <= N / 2; ++k) { const double f = k * sr / N; if (f >= fc / std::sqrt (2.0) && f < fc * std::sqrt (2.0)) s += P[k]; } out.push_back (10 * std::log10 (s / segs + 1e-30)); }
    return out;
}
static void render (Engine& e, std::vector<float>& l, std::vector<float>& r, int block)
{
    for (size_t i = 0; i < l.size(); i += (size_t) block) { const int n = (int) std::min<size_t> ((size_t) block, l.size() - i); float* io[2] = { l.data() + i, r.data() + i }; e.process (io, 2, n); }
}
static double rmsDb (const std::vector<float>& x, size_t a) { double s = 0; for (size_t i = a; i < x.size(); ++i) s += (double) x[i] * x[i]; return 10 * std::log10 (s / (x.size() - a) + 1e-30); }

int main (int argc, char** argv)
{
    const double sr = 48000; const size_t L = (size_t) sr * 60;
    const char* names[3] = { "blanc", "rose", "brun" }; const double want[3] = { 0, -3.01, -6.02 };
    // 1. niveaux et pentes
    for (int t = 0; t < 3; ++t)
    {
        Engine e; e.prepare (sr); Params p; p.on = true; p.type = t; p.levelDb = -18; p.width = 1; p.keepInput = false; e.setParams (p); e.reset();
        std::vector<float> l (L, 0.f), r (L, 0.f); render (e, l, r, 512);
        const double lv = rmsDb (l, (size_t) sr), lvR = rmsDb (r, (size_t) sr);
        const auto oc = octaves (l, sr);
        // pente par régression sur les octaves 250 Hz .. 4 kHz
        double sx = 0, sy = 0, sxx = 0, sxy = 0; int m = 0; for (size_t k = 1; k + 1 < oc.size(); ++k) { const double x = (double) k, y = oc[k] - 10 * std::log10 (std::pow (2.0, (double) k)); sx += x; sy += y; sxx += x * x; sxy += x * y; ++m; }
        const double slope = (m * sxy - sx * sy) / (m * sxx - sx * sx);    // dB par octave (puissance par Hz)
        line (std::abs (lv + 18) < 0.5 && std::abs (lvR + 18) < 0.5 && std::abs (slope - want[t]) < 0.5,
              F ("bruit %s : niveau L %.2f / R %.2f dBFS (demandé -18), pente %.2f dB/octave (attendu %.2f)", names[t], lv, lvR, slope, want[t]));
    }
    // 2. couleur : ±1 change la pente d'environ ±3 dB/oct sans changer le niveau de plus de 1 dB
    {
        double sl[2], lvs[2]; const float cols[2] = { -1.f, 1.f };
        for (int c = 0; c < 2; ++c)
        { Engine e; e.prepare (sr); Params p; p.on = true; p.type = 0; p.color = cols[c]; p.keepInput = false; e.setParams (p); e.reset();
          std::vector<float> l (sr * 20, 0.f), r (sr * 20, 0.f); render (e, l, r, 256); auto oc = octaves (l, sr); sl[c] = (oc[5] - oc[1]) / 4 - 3.01; lvs[c] = rmsDb (l, (size_t) sr); }
        line (sl[0] < -1.5 && sl[1] > 1.5 && std::abs (lvs[0] + 18) < 1.5 && std::abs (lvs[1] + 18) < 1.5,
              F ("couleur -1 / +1 : pente entre 250 Hz et 4 kHz %.2f / %.2f dB/oct, niveau %.2f / %.2f dBFS", sl[0] + 3.01 - 3.01, sl[1], lvs[0], lvs[1]));
    }
    // 3. mono / stéréo : corrélation L/R
    {
        double cor[2];
        for (int w = 0; w < 2; ++w)
        { Engine e; e.prepare (sr); Params p; p.on = true; p.type = 1; p.width = (float) w; p.keepInput = false; e.setParams (p); e.reset();
          std::vector<float> l (sr * 10, 0.f), r (sr * 10, 0.f); render (e, l, r, 512);
          double lr = 0, ll = 0, rr = 0; for (size_t i = sr; i < l.size(); ++i) { lr += (double) l[i] * r[i]; ll += (double) l[i] * l[i]; rr += (double) r[i] * r[i]; } cor[w] = lr / std::sqrt (ll * rr); }
        line (cor[0] > 0.999 && std::abs (cor[1]) < 0.02, F ("largeur 0 : corrélation L/R %.4f (mono) ; largeur 100 %% : %.4f (décorrélé)", cor[0], cor[1]));
    }
    // 4. démarrage muet, puis activation / changement de couleur sans clic, signal d'entrée intact
    {
        Engine e; e.prepare (sr); Params p; e.setParams (p); e.reset();
        std::vector<float> l (sr * 4), r (sr * 4); for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = (float) (0.3 * std::sin (2 * M_PI * 110 * i / sr));
        const auto src = l;
        double silentErr = 0; double maxJump = 0;
        for (size_t i = 0; i < l.size(); i += 256)
        {
            p.on = i >= sr; if (i >= sr * 2) p.type = 2; if (i >= sr * 3) p.type = 0; p.levelDb = -6; e.setParams (p);
            float* io[2] = { l.data() + i, r.data() + i }; e.process (io, 2, 256);
        }
        for (size_t i = 0; i < sr; ++i) silentErr = std::max (silentErr, (double) std::abs (l[i] - src[i]));
        // saut maximal du bruit ajouté sur 1 échantillon pendant les 5 ms qui suivent chaque événement, comparé au régime établi
        auto jump = [&] (size_t a, size_t b) { double m = 0; for (size_t i = a + 1; i < b; ++i) m = std::max (m, (double) std::abs ((l[i] - src[i]) - (l[i - 1] - src[i - 1]))); return m; };
        const double j0 = jump (sr, sr + 240), steady = jump ((size_t) (sr * 1.5), (size_t) (sr * 1.9)), j1 = jump (sr * 2, sr * 2 + 2400);
        maxJump = std::max (j0, j1);
        line (silentErr == 0 && maxJump <= steady * 1.05, F ("muet au départ (entrée inchangée, écart %.1g) ; activation et changement de couleur : saut max %.3f ≤ régime établi %.3f", silentErr, maxJump, steady));
    }
    // 5. stabilité longue et fréquences d'échantillonnage
    {
        bool ok = true; double worst = 0, drift = 0;
        for (double s : { 44100.0, 48000.0, 96000.0 })
        { Engine e; e.prepare (s); Params p; p.on = true; p.type = 2; p.levelDb = 0; p.keepInput = false; e.setParams (p); e.reset();
          std::vector<float> l ((size_t) s * 120, 0.f), r ((size_t) s * 120, 0.f); render (e, l, r, 1024);
          double dc = 0; for (size_t i = 0; i < l.size(); ++i) { ok &= std::isfinite (l[i]); worst = std::max (worst, (double) std::abs (l[i])); dc += l[i]; }
          drift = std::max (drift, std::abs (dc / l.size())); }
        line (ok && worst <= 1.0 && drift < 0.05, F ("bruit brun au niveau maximum (0 dBFS RMS), 120 s à 44,1 / 48 / 96 kHz : aucun NaN, crête max %.2f (jamais au-dessus de 0 dBFS), composante continue moyenne %.4f", worst, drift));
    }
    std::printf ("%s\n", fails ? "ÉCHECS" : "Tous les tests Noise passent");
    if (argc > 1) { FILE* f = std::fopen (argv[1], "a"); if (f) { std::fputs (rep.c_str(), f); std::fclose (f); } }
    return fails ? 1 : 0;
}
