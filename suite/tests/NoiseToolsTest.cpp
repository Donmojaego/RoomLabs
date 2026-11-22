// Noise Tools — corpus synthétique reproductible (graine fixe) et mesures. Sortie : rapport Markdown.
#include "../plugins/noise_tools/NoiseToolsDSP.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
using namespace rl::nt;
static int fails = 0; static std::string rep;
static void line (bool ok, const std::string& s) { if (! ok) ++fails; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; std::printf ("%s %s\n", ok ? "OK " : "ÉCHEC", s.c_str()); }
static char buf[512];
#define F(...) (std::snprintf (buf, sizeof buf, __VA_ARGS__), std::string (buf))

// signal « musical » : notes harmoniques avec enveloppes, vibrato, silences, et attaques franches
static std::vector<float> music (double sr, double secs, unsigned seed)
{
    std::mt19937 rng (seed); std::uniform_real_distribution<double> U (0, 1);
    std::vector<float> x ((size_t) (sr * secs), 0.f);
    double t0 = 0.2;
    while (t0 < secs - 0.6)
    {
        const double dur = 0.25 + 0.5 * U (rng), f0 = 110 * std::pow (2.0, std::floor (U (rng) * 24) / 12.0), amp = 0.15 + 0.25 * U (rng);
        const int s0 = (int) (t0 * sr), n = (int) (dur * sr);
        double ph = 0;
        for (int i = 0; i < n && s0 + i < (int) x.size(); ++i)
        {
            const double t = i / sr, env = std::min (1.0, t / 0.008) * std::exp (-t * 3.0) * (i > n - 400 ? (n - i) / 400.0 : 1.0);
            ph += 2 * M_PI * f0 * (1 + 0.004 * std::sin (2 * M_PI * 5.5 * t)) / sr;
            double s = 0; for (int h = 1; h <= 8; ++h) s += std::sin (h * ph) / (h * 1.3);
            x[(size_t) (s0 + i)] += (float) (amp * env * s * 0.6);
        }
        t0 += dur + (U (rng) < 0.3 ? 0.35 : 0.05);
    }
    return x;
}
static double rmsDb (const std::vector<float>& a, size_t s, size_t e) { double q = 0; for (size_t i = s; i < e; ++i) q += a[i] * a[i]; return 10 * std::log10 (q / std::max<size_t> (1, e - s) + 1e-20); }
static double errDb (const std::vector<float>& ref, const std::vector<float>& y, int lat, size_t s, size_t e)
{ double q = 0, r = 0; for (size_t i = s; i < e; ++i) { const double d = y[i + (size_t) lat] - ref[i]; q += d * d; r += ref[i] * ref[i]; } return 10 * std::log10 ((q + 1e-20) / (r + 1e-20)); }

static std::vector<float> run (Engine& e, const std::vector<float>& in, int block, const Params& p)
{
    std::vector<float> y (in.size() + (size_t) Engine::latency() + 4096, 0.f);
    std::copy (in.begin(), in.end(), y.begin());
    e.setParams (p);
    for (size_t i = 0; i < y.size(); i += (size_t) block) { float* io[1] = { y.data() + i }; e.process (io, 1, (int) std::min<size_t> ((size_t) block, y.size() - i)); }
    return y;
}

int main (int argc, char** argv)
{
    rep = "\n## Noise Tools — mesures sur corpus synthétique (graine 7)\n\n";
    const double sr = 48000; const int L = Engine::latency();
    auto clean = music (sr, 8, 7);
    std::mt19937 rng (11); std::normal_distribution<double> G (0, 1);
    // 1. transparence : tout coupé → sortie = entrée retardée
    {
        Engine e; e.prepare (sr, 1); Params p; p.dnOn = p.dcOn = p.dpOn = false;
        auto y = run (e, clean, 256, p);
        const double er = errDb (clean, y, L, 0, clean.size() - 4096);
        line (er < -90, F ("transparence (traitements coupés) : erreur %.1f dB, latence %d échantillons (%.1f ms à 48 kHz)", er, L, L * 1000.0 / sr));
    }
    // 2. souffle : bruit blanc -40 dBFS
    {
        auto noisy = clean; for (auto& v : noisy) v += (float) (0.01 * G (rng));
        Engine e; e.prepare (sr, 1); Params p; p.dcOn = false; p.dnAmount = 18;
        auto y = run (e, noisy, 512, p);
        // zones silencieuses du signal propre : niveau de bruit résiduel
        double nin = 0, nout = 0; size_t cnt = 0;
        // silences longs seulement (> 0,25 s, 60 ms retirés aux bords) : souffle résiduel entre les phrases
        for (size_t i = sr * 2; i + 4096 < clean.size();)
        {
            size_t j = i; while (j < clean.size() && std::abs (clean[j]) < 1e-6) ++j;
            if (j - i > sr * 0.25) for (size_t q = i + 2880; q < j - 2880; ++q) { nin += (noisy[q] - clean[q]) * (noisy[q] - clean[q]); nout += (double) y[q + L] * y[q + L]; ++cnt; }
            i = j + 1;
        }
        const double red = 10 * std::log10 (nin / std::max (1e-20, nout));
        const double e0 = errDb (clean, noisy, 0, sr * 2, clean.size() - 4096), e1 = errDb (clean, y, L, sr * 2, clean.size() - 4096);
        line (red > 12 && e1 < e0 - 4, F ("Denoise (souffle -40 dBFS) : bruit dans les silences -%.1f dB, erreur totale %.1f → %.1f dB", red, e0, e1));
    }
    // 3. ronflement 50 Hz + harmoniques
    {
        auto hum = clean; for (size_t i = 0; i < hum.size(); ++i) { double s = 0; for (int k = 1; k <= 5; ++k) s += std::sin (2 * M_PI * 50 * k * i / sr) / k; hum[i] += (float) (0.03 * s); }
        Engine e; e.prepare (sr, 1); Params p; p.dnOn = false; p.dcOn = false; p.hum = 1;
        auto y = run (e, hum, 128, p);
        // ronflement résiduel : projection de l'erreur sur chaque harmonique (Goertzel sur 5 s)
        auto humAmp = [&] (const std::vector<float>& v, int off) { double tot = 0; for (int k = 1; k <= 5; ++k) { double re = 0, im = 0;
            for (size_t i = sr; i < sr * 6; ++i) { const double d = v[i + (size_t) off] - clean[i]; re += d * std::cos (2 * M_PI * 50 * k * i / sr); im += d * std::sin (2 * M_PI * 50 * k * i / sr); }
            tot += re * re + im * im; } return 10 * std::log10 (tot + 1e-20); };
        const double e0 = humAmp (hum, 0), e1 = humAmp (y, L);
        line (e1 < e0 - 25, F ("Hum 50 Hz (5 harmoniques, -30 dBFS) : ronflement résiduel -%.1f dB", e0 - e1));
    }
    // 4. clics : 40 impulsions courtes
    {
        auto ck = clean; std::uniform_int_distribution<int> P ((int) sr, (int) (clean.size() - sr)); std::uniform_real_distribution<double> U (-1, 1);
        std::vector<int> pos; for (int k = 0; k < 40; ++k) { const int p0 = P (rng); pos.push_back (p0); const int w = 1 + k % 6; for (int j = 0; j < w; ++j) ck[(size_t) (p0 + j)] += (float) (0.4 * (k % 2 ? 1 : -1) * (1 - j / (double) w) + 0.05 * U (rng)); }
        Engine e; e.prepare (sr, 1); Params p; p.dnOn = false; p.dcOn = true;
        auto y = run (e, ck, 512, p);
        int fixedN = 0; for (int p0 : pos) { double m = 0; for (int j = -2; j < 8; ++j) m = std::max (m, (double) std::abs (y[(size_t) (p0 + j + L)] - clean[(size_t) (p0 + j)])); if (m < 0.1) ++fixedN; }
        Engine e2; e2.prepare (sr, 1); auto yc = run (e2, clean, 512, p);    // fausses détections sur le signal propre
        const double e0 = errDb (clean, ck, 0, sr, clean.size() - 4096), e1 = errDb (clean, y, L, sr, clean.size() - 4096), ec = errDb (clean, yc, L, sr, clean.size() - 4096);
        line (fixedN >= 34 && e1 < e0 - 10 && e2.totalClicks <= 4, F ("De-click : %d/40 clics réparés (erreur %.1f → %.1f dB) ; sur le signal propre %ld détection(s), écart %.1f dB", fixedN, e0, e1, e2.totalClicks, ec));
    }
    // 5. écrêtage dur à 50 % de la crête
    {
        float pk = 0; for (float v : clean) pk = std::max (pk, std::abs (v));
        auto cl = clean; const float lim = pk * 0.7f; for (auto& v : cl) v = std::clamp (v, -lim, lim);
        Engine e; e.prepare (sr, 1); Params p; p.dnOn = false; p.dcOn = false; p.dpOn = true; p.dpMarginDb = 0;
        auto y = run (e, cl, 512, p);
        const double e0 = errDb (clean, cl, 0, sr, clean.size() - 4096), e1 = errDb (clean, y, L, sr, clean.size() - 4096);
        Engine e2; e2.prepare (sr, 1); auto yc = run (e2, clean, 512, p);    // fausses détections sur le signal propre
        const double ec = errDb (clean, yc, L, sr, clean.size() - 4096);
        line (e1 < e0 - 20 && ec < -90, F ("De-clip (écrêtage à -3 dB de la crête) : erreur %.1f → %.1f dB, %ld plateaux reconstruits ; signal propre : %ld plateau(x), écart %.1f dB", e0, e1, e.totalClipRuns, e2.totalClipRuns, ec));
    }
    // 6. robustesse : fréquences, tailles de bloc, silence, impulsion, bruit fort, stéréo
    {
        bool ok = true; double worst = 0;
        for (double s : { 44100.0, 48000.0, 96000.0 })
            for (int b : { 64, 128, 256, 512, 1024 })
            {
                Engine e; e.prepare (s, 2); Params p; p.dpOn = true; p.hum = 2;
                std::vector<float> l ((size_t) (s * 1.5)), r (l.size());
                for (size_t i = 0; i < l.size(); ++i) { l[i] = (float) (0.9 * std::sin (i * 0.01) + 0.3 * G (rng)); r[i] = i == 1000 ? 1.f : 0.f; }
                for (size_t i = 0; i < l.size(); i += (size_t) b) { float* io[2] = { l.data() + i, r.data() + i }; e.process (io, 2, (int) std::min<size_t> ((size_t) b, l.size() - i)); }
                for (size_t i = 0; i < l.size(); ++i) { ok &= std::isfinite (l[i]) && std::isfinite (r[i]); worst = std::max (worst, (double) std::abs (l[i])); }
            }
        line (ok && worst < 4, F ("44,1 / 48 / 96 kHz × blocs 64-1024, stéréo, impulsion, bruit fort : aucun NaN, crête max %.2f", worst));
    }
    // 7. bypass et A/B sans clic
    {
        Engine e; e.prepare (sr, 1); Params p; std::vector<float> y (sr * 2); double maxStep = 0; float prev = 0;
        for (size_t i = 0; i < y.size(); ++i) y[i] = (float) (0.3 * std::sin (2 * M_PI * 220 * i / sr));
        for (size_t i = 0; i < y.size(); i += 256) { p.bypass = (i / 256) % 40 > 20; p.compare = (i / 256) % 30 > 15; e.setParams (p); float* io[1] = { y.data() + i }; e.process (io, 1, 256); }
        for (size_t i = (size_t) L; i < y.size(); ++i) { maxStep = std::max (maxStep, (double) std::abs (y[i] - prev)); prev = y[i]; }
        line (maxStep < 0.05, F ("bypass et comparaison A/B en marche : saut max entre échantillons %.4f (sinus 220 Hz : 0,0086 attendu)", maxStep));
    }
    // 8. charge processeur (information) : 20 s stéréo 48 kHz, les trois traitements + hum
    {
        Engine e; e.prepare (48000, 2); Params p; p.dpOn = true; p.hum = 1; e.setParams (p);
        std::vector<float> l (512), r (512); std::normal_distribution<float> n (0, .1f); float* io[2] = { l.data(), r.data() };
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < 48000 * 20 / 512; ++b) { for (int i = 0; i < 512; ++i) { l[(size_t) i] = n (rng); r[(size_t) i] = n (rng); } e.process (io, 2, 512); }
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        line (s < 20 * 0.5, F ("charge : 20 s stéréo 48 kHz traitées en %.2f s, soit %.1f %% d'un cœur (tout activé, blocs 512)", s, 100 * s / 20));
    }
    std::printf ("%s\n", fails ? "ÉCHECS" : "Tous les tests Noise Tools passent");
    if (argc > 1) { FILE* f = std::fopen (argv[1], "a"); if (f) { std::fputs (rep.c_str(), f); std::fclose (f); } }
    return fails ? 1 : 0;
}
