// Tests du moteur African Riddim : 54 instruments, justesse, niveaux calibrés, stabilité, CPU.
// Avec --calibrate <fichier> : mesure le niveau de chaque instrument et écrit calibration.inc.
#include "engine/AfricanEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace afr;
static std::string rep; static int fails = 0;
static void line (bool ok, const std::string& s) { std::printf ("%s  %s\n", ok ? "OK " : "ÉCHEC", s.c_str()); rep += std::string ("- [") + (ok ? "OK" : "ÉCHEC") + "] " + s + "\n"; if (! ok) ++fails; }
template <typename... A> static std::string F (const char* f, A... a) { char b[600]; std::snprintf (b, sizeof b, f, a...); return b; }

struct Out { std::vector<float> l, r; };
static Out play (Engine& e, int note, float vel, double sr, double holdS, double totalS, int block = 256)
{
    Out o; const size_t N = (size_t) (totalS * sr); o.l.assign (N, 0); o.r.assign (N, 0);
    e.noteOn (note, vel); bool off = false;
    for (size_t i = 0; i < N; i += (size_t) block)
    {
        if (! off && i >= holdS * sr) { e.noteOff (note); off = true; }
        const int n = (int) std::min<size_t> ((size_t) block, N - i); e.process (o.l.data() + i, o.r.data() + i, n);
    }
    return o;
}
// loudness simple : RMS (dB) sur 600 ms après l'attaque, pondéré par un passe-haut doux (écoute)
static double loud (const Out& o, double sr)
{
    double s = 0; size_t n = std::min (o.l.size(), (size_t) (0.6 * sr)); float hp = 0, px = 0;
    for (size_t i = 0; i < n; ++i) { const float x = 0.5f * (o.l[i] + o.r[i]); hp = 0.995f * (hp + x - px); px = x; s += (double) hp * hp; }
    return 10 * std::log10 (s / n + 1e-20);
}
static double peak (const Out& o) { double p = 0; for (size_t i = 0; i < o.l.size(); ++i) p = std::max (p, (double) std::max (std::abs (o.l[i]), std::abs (o.r[i]))); return p; }
// fondamentale par autocorrélation (instruments à hauteur nette)
static double pitch (const std::vector<float>& x, double sr, size_t a, size_t len, double fmin, double fmax)
{
    double best = 0; int bl = 0; const int l0 = (int) (sr / fmax), l1 = (int) (sr / fmin);
    std::vector<double> c ((size_t) l1 + 2, 0);
    double e0 = 0; for (size_t i = a; i < a + len; ++i) e0 += (double) x[i] * x[i];
    for (int L = l0; L <= l1; ++L) { double s = 0, e1 = 0; for (size_t i = a; i < a + len; ++i) { s += (double) x[i] * x[i + (size_t) L]; e1 += (double) x[i + (size_t) L] * x[i + (size_t) L]; } c[(size_t) L] = s / std::sqrt (e0 * e1 + 1e-20); }
    for (int L = l0; L <= l1; ++L) if (c[(size_t) L] > best * 1.0001 && c[(size_t) L] > 0.0) { best = c[(size_t) L]; bl = L; }
    // premier pic proche du maximum (évite l'octave en dessous)
    for (int L = l0 + 1; L < l1; ++L) if (c[(size_t) L] > 0.9 * best && c[(size_t) L] >= c[(size_t) L - 1] && c[(size_t) L] >= c[(size_t) L + 1]) { bl = L; break; }
    if (bl <= l0 || bl >= l1) return 0;
    const double y0 = c[(size_t) bl - 1], y1 = c[(size_t) bl], y2 = c[(size_t) bl + 1]; const double d = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2 + 1e-12);
    return sr / (bl + d);
}

int main (int argc, char** argv)
{
    const double sr = 48000;
    const bool calib = argc > 2 && std::strcmp (argv[1], "--calibrate") == 0;
    const double target = -24.0;   // dB RMS d'une note jouée à mi-force (crêtes sous -3 dBFS au volume 0 dB)
    std::vector<double> lv (54), pk (54);
    rep += "\n## African Riddim — moteur (54 instruments)\n\n";
    int stableOk = 0; double worstPk = 0;
    for (int i = 0; i < 54; ++i)
    {
        Engine e; e.prepare (sr); Params p; p.instrument = i; p.space = 0; e.setParams (p);
        auto o = play (e, 60, 0.8f, sr, 0.6, 1.4);
        lv[(size_t) i] = loud (o, sr);
        bool fin = true; for (auto x : o.l) fin &= std::isfinite (x);
        pk[(size_t) i] = peak (o); stableOk += fin && peak (o) < 1.5; worstPk = std::max (worstPk, peak (o));
    }
    if (calib)
    {
        FILE* f = std::fopen (argv[2], "w");
        // gain = cible RMS, mais sans laisser une crête dépasser -3 dBFS (instruments très percussifs)
        for (int i = 0; i < 54; ++i)
        {
            const double raw = lv[(size_t) i] - Engine::calibrationDb (i), rawPk = 20 * std::log10 (pk[(size_t) i]) - Engine::calibrationDb (i);
            const double g = std::min (target - raw, -3.0 - rawPk);
            std::fprintf (f, "%.2ff%s // %s — %s\n", (float) g, i < 53 ? "," : "", kInst[i].country, kInst[i].name);
        }
        std::fclose (f); std::printf ("calibration écrite\n"); return 0;
    }
    double lo = 1e9, hi = -1e9; int lowI = 0, hiI = 0;
    for (int i = 0; i < 54; ++i) { if (lv[(size_t) i] < lo) { lo = lv[(size_t) i]; lowI = i; } if (lv[(size_t) i] > hi) { hi = lv[(size_t) i]; hiI = i; } }
    int near = 0; for (int i = 0; i < 54; ++i) near += std::abs (lv[(size_t) i] - target) <= 2.0;
    line (near >= 48 && hi - lo < 8.0, F ("niveaux calibrés : %d/54 instruments à ±2 dB de la cible (%.0f dB RMS) ; extrêmes %s %.1f dB et %s %.1f dB (crête limitée à -3 dBFS pour les attaques très sèches)", near, target, kInst[lowI].name, lo, kInst[hiI].name, hi));
    line (stableOk == 54 && worstPk < 1.0, F ("54 instruments : aucun NaN, crête max %.2f (volume 0 dB, une note à mi-force)", worstPk));
    // justesse des instruments à hauteur définie (do central, la 440, quelques notes)
    {
        int ok = 0, tot = 0; std::string bad;
        for (int i = 0; i < 54; ++i)
        {
            const auto m = kInst[i].model; if (m == Model::Drum || (m == Model::Bar && kInst[i].special)) continue;
            for (int note : { 57, 64, 69 })
            {
                Engine e; e.prepare (sr); Params p; p.instrument = i; p.space = 0; e.setParams (p);
                auto o = play (e, note, 0.7f, sr, 0.8, 0.9);
                const double want = 440.0 * std::pow (2.0, (note + 12 * kInst[i].octave - 69) / 12.0);
                const double f = pitch (o.l, sr, (size_t) (0.15 * sr), 4096, want / 1.6, want * 1.6);
                const double cents = f > 0 ? 1200 * std::log2 (f / want) : 9999;
                ++tot; if (std::abs (cents) < 15) ++ok; else bad += F (" %s/%d:%.0fc", kInst[i].name, note, cents);
            }
        }
        line (ok >= tot - 3, F ("justesse : %d/%d notes à moins de 15 cents%s", ok, tot, bad.c_str()));
    }
    // fin de note : relâchement, voix libérées
    {
        Engine e; e.prepare (sr); Params p; p.instrument = 19; p.releaseMs = 200; e.setParams (p);   // goge (archet, son tenu)
        auto o = play (e, 62, 0.8f, sr, 0.5, 1.6);
        double tail = 0; for (size_t i = (size_t) (1.3 * sr); i < o.l.size(); ++i) tail = std::max (tail, (double) std::abs (o.l[i]));
        line (tail < 1e-3 && e.fb.voices.load() == 0, F ("son tenu (goge) : relâchement 200 ms → reste %.1g après 0,8 s, voix libérées", tail));
    }
    // polyphonie, vol de voix, tailles de bloc, fréquences
    {
        bool ok = true; double pk = 0;
        for (double s : { 44100.0, 48000.0, 96000.0 })
            for (int blk : { 32, 512, 1024 })
            {
                Engine e; e.prepare (s); Params p; p.instrument = 9; p.space = 0.6f; e.setParams (p);
                std::vector<float> l ((size_t) s * 3), r ((size_t) s * 3);
                for (size_t i = 0; i < l.size(); i += (size_t) blk)
                {
                    if ((i / blk) % 3 == 0) e.noteOn (48 + (int) ((i / blk) % 37), 0.9f);
                    if ((i / blk) % 5 == 0) e.noteOff (48 + (int) ((i / blk) % 37));
                    const int n = (int) std::min<size_t> ((size_t) blk, l.size() - i); e.process (l.data() + i, r.data() + i, n);
                }
                for (auto x : l) { ok &= std::isfinite (x); pk = std::max (pk, (double) std::abs (x)); }
            }
        line (ok && pk <= 0.9, F ("44,1 / 48 / 96 kHz × blocs 32-1024, notes en rafale (vol de voix) : aucun NaN, crête %.2f", pk));
    }
    // CPU : 16 voix en continu
    {
        Engine e; e.prepare (48000); Params p; p.instrument = 9; p.space = 0.5f; e.setParams (p);
        std::vector<float> l (512), r (512); const auto t0 = std::chrono::steady_clock::now(); int blocks = 48000 * 10 / 512;
        for (int b = 0; b < blocks; ++b) { if (b % 6 == 0) for (int k = 0; k < 16; ++k) e.noteOn (40 + (b + k * 3) % 40, 0.8f); e.process (l.data(), r.data(), 512); }
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        line (s < 10 * 0.25, F ("charge : 16 voix (kora) + réverbération, 10 s traitées en %.2f s, soit %.1f %% d'un cœur", s, 10 * s));
    }
    std::printf ("%s\n", fails ? "ÉCHECS" : "Tous les tests du moteur passent");
    if (argc > 1 && ! calib) { FILE* f = std::fopen (argv[1], "a"); if (f) { std::fputs (rep.c_str(), f); std::fclose (f); } }
    return fails ? 1 : 0;
}
