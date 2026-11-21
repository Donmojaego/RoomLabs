// =============================================================================
// Windup Dogs — test du moteur (sans JUCE).
//   g++ -O2 -std=c++20 -Isource -Itests tests/EngineTest.cpp source/engine/WindupEngine.cpp -o engine-test && ./engine-test [RAPPORT.md]
// =============================================================================
#include "engine/WindupEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct P { const char* name; int root; float v[19]; };
static const P presets[] = {
#include "factory_presets.inc"
};
#include "params.inc"

static const double SR = 48000;
static double hzOf (double s) { return 440.0 * std::pow (2.0, (s - 69.0) / 12.0); }
static double rms (const std::vector<float>& x, double t0, double t1)
{
    double s = 0; int n = 0;
    for (int i = (int) (t0 * SR); i < (int) (t1 * SR) && i < (int) x.size(); ++i, ++n) s += x[(size_t) i] * x[(size_t) i];
    return std::sqrt (s / std::max (1, n));
}
static double pitchOf (const float* x, int n, double expect)
{
    const int lo = (int) (SR / (expect * 1.4)), hi = (int) (SR / (expect / 1.4));
    if (hi + 2 >= n) return 0;
    std::vector<double> r ((size_t) hi + 3);
    for (int lag = lo - 1; lag <= hi + 1; ++lag)
    { double s = 0, a = 0, b = 0; for (int i = 0; i + lag < n; ++i) { s += x[i] * x[i + lag]; a += x[i] * x[i]; b += x[i + lag] * x[i + lag]; } r[(size_t) lag] = s / std::sqrt (a * b + 1e-20); }
    int best = lo; for (int l = lo; l <= hi; ++l) if (r[(size_t) l] > r[(size_t) best]) best = l;
    const double a = r[(size_t) best - 1], b = r[(size_t) best], c = r[(size_t) best + 1];
    return SR / (best + 0.5 * (a - c) / (a - 2 * b + c + 1e-20));
}
// fréquence du partiel le plus fort près de la note attendue (±80 cents), pour les sons inharmoniques
static double peakHz (const float* x, int n, double expect)
{
    double best = expect, bm = -1;
    for (double c = -80; c <= 80; c += 0.25)
    {
        const double f = expect * std::pow (2.0, c / 1200.0); double re = 0, im = 0;
        for (int i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (6.283185307 * i / (n - 1)); re += x[i] * w * std::cos (6.283185307 * f * i / SR); im += x[i] * w * std::sin (6.283185307 * f * i / SR); }
        const double m = re * re + im * im; if (m > bm) { bm = m; best = f; }
    }
    return best;
}
static std::vector<float> play (wd::Params q, int note, double holdS, double totalS, wd::WindupEngine* keep = nullptr, double ppq0 = -1)
{
    wd::WindupEngine local; auto& e = keep ? *keep : local;
    e.prepare (SR, 512); e.setParams (q); e.setTransport (120, ppq0);
    std::vector<float> L ((size_t) (totalS * SR)), R (L.size());
    const int hold = (int) (holdS * SR);
    e.noteOn (note, 0.9f);
    for (int p = 0; p < (int) L.size(); p += 256)
    {
        if (p <= hold && p + 256 > hold) e.noteOff (note);
        if (ppq0 >= 0) e.setTransport (120, ppq0 + p / SR * 2.0);
        e.render (L.data() + p, R.data() + p, std::min (256, (int) L.size() - p));
    }
    return L;
}
static wd::Params dry (wd::Inst i)
{
    wd::Params q; q.inst = i; q.space = 0; q.echo = 0; q.wobble = 0; q.meca = 0; q.tone = 1; q.levelDb = -6; q.tickOn = false; q.springOn = false;
    q.attackMs = 5; q.decayMs = 3000; q.sustain = 0.8f; q.releaseMs = 300; return q;
}

int main (int argc, char** argv)
{
    std::string rep = "# Windup Dogs — rapport des tests du moteur\n\n";
    int failures = 0; char buf[600];
    auto line = [&] (bool ok, const std::string& s) { if (! ok) ++failures; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; };

    rep += "## Presets d'usine (note de référence, vélocité 0,9)\n\n| Preset | Note | Crête (dBFS) | Silence après relâche | Résultat |\n|---|---|---|---|---|\n";
    const int count = (int) (sizeof presets / sizeof presets[0]);
    for (int i = 0; i < count; ++i)
    {
        auto q = toParams (presets[i].v);
        auto L = play (q, presets[i].root, 1.0, 14.0);
        double pk = 0; bool finite = true;
        for (float x : L) { pk = std::max (pk, (double) std::abs (x)); finite &= std::isfinite (x); }
        const double pkDb = 20 * std::log10 (pk + 1e-12), tail = 20 * std::log10 (rms (L, 13.0, 14.0) + 1e-12);
        const bool ok = finite && pkDb > -16 && pkDb < -1 && tail < -60;
        if (! ok) ++failures;
        std::snprintf (buf, sizeof buf, "| %s | %d | %.1f | %.0f dB | %s |\n", presets[i].name, presets[i].root, pkDb, tail, ok ? "OK" : "ÉCHEC");
        rep += buf;
    }

    rep += "\n## Justesse (à sec)\n\n";
    {
        struct T { const char* n; wd::Inst i; int note; };
        const T ts[] = { { "Boîte à musique", wd::Inst::MusicBox, 72 }, { "Piano jouet", wd::Inst::ToyPiano, 67 }, { "Clochettes", wd::Inst::Bells, 72 }, { "Fanfare", wd::Inst::Fanfare, 64 } };
        for (auto& t : ts)
        {
            auto L = play (dry (t.i), t.note, 1.0, 1.0);
            const double hz = t.i == wd::Inst::Bells ? peakHz (L.data() + (int) (0.15 * SR), (int) (0.25 * SR), hzOf (t.note))
                                                     : pitchOf (L.data() + (int) (0.15 * SR), (int) (0.25 * SR), hzOf (t.note)), cents = 1200 * std::log2 (hz / hzOf (t.note));
            std::snprintf (buf, sizeof buf, "%s (note %d) : %+.1f cents", t.n, t.note, cents);
            line (std::abs (cents) < 8, buf);
        }
    }

    rep += "\n## Mécanisme\n\n";
    {
        // ressort : après le temps de détente, la hauteur a baissé (≈ -70 cents), puis la clé remonte
        auto q = dry (wd::Inst::Fanfare); q.springOn = true; q.springTime = 2;
        wd::WindupEngine e; e.prepare (SR, 512); e.setParams (q); e.setTransport (120, -1);
        std::vector<float> L ((size_t) SR), R (L.size());
        e.noteOn (64, 0.9f);
        for (int k = 0; k < 3; ++k) e.render (L.data(), R.data(), (int) SR);
        const double low = pitchOf (L.data() + 2000, 12000, hzOf (64)), droop = 1200 * std::log2 (low / hzOf (64));
        std::snprintf (buf, sizeof buf, "ressort détendu : tension %.2f, hauteur %+.0f cents (attendu vers -70)", e.tension(), droop);
        line (e.tension() < 0.01f && droop < -50 && droop > -95, buf);
        const int w0 = e.fb.winds.load(); e.wind();
        e.render (L.data(), R.data(), (int) SR);
        const double back = pitchOf (L.data() + 30000, 12000, hzOf (64)), bc = 1200 * std::log2 (back / hzOf (64));
        std::snprintf (buf, sizeof buf, "clé : remontage en moins d'1 s (tension %.2f), hauteur revenue à %+.1f cents", e.tension(), bc);
        line (e.tension() > 0.99f && std::abs (bc) < 6 && e.fb.winds.load() == w0 + 1, buf);
    }
    {
        // tic-tac calé sur l'hôte : 1/8 à 120 BPM = 8 par seconde
        auto q = dry (wd::Inst::MusicBox); q.tickOn = true; q.tickDiv = 1; q.tickLevel = 0.5f;
        wd::WindupEngine e;
        auto L = play (q, 72, 2.0, 2.0, &e, 0.0);
        std::snprintf (buf, sizeof buf, "tic-tac au tempo de l'hôte (1/8, 120 BPM) : %d en 2 s (attendu 8)", e.fb.ticks.load());
        line (e.fb.ticks.load() >= 8 && e.fb.ticks.load() <= 9, buf);
        // swing : le temps faible est retardé
        auto qs = q; qs.swing = 0.6f; qs.tickDiv = 1;
        wd::WindupEngine s; s.prepare (SR, 512); s.setParams (qs);
        std::vector<float> A (256), B (256); int firstOdd = -1, n0 = 0;
        for (int p = 0; p < (int) SR; p += 256) { s.setTransport (120, p / SR * 2.0); s.render (A.data(), B.data(), 256); if (s.fb.ticks.load() == 2 && firstOdd < 0) firstOdd = p; n0 = s.fb.ticks.load(); }
        const double at = firstOdd / SR;      // 1/8 à 120 BPM = 0,25 s ; avec swing 0,6 -> 0,25 + 0,6 × 0,5 × 0,25 = 0,325 s
        std::snprintf (buf, sizeof buf, "swing 60 %% : deuxième tic à %.3f s (attendu ≈ 0,325 s)", at);
        line (std::abs (at - 0.325) < 0.012 && n0 >= 3, buf);
    }
    {
        // écho au tempo : croche pointée = 375 ms à 120 BPM
        auto q = dry (wd::Inst::ToyPiano); q.echo = 0.6f; q.decayMs = 120;
        auto L = play (q, 72, 0.05, 1.0);
        const double a = rms (L, 0.36, 0.42), b = rms (L, 0.25, 0.32);
        std::snprintf (buf, sizeof buf, "écho à la croche pointée : %.1f dB vers 375 ms contre %.1f dB avant", 20 * std::log10 (a + 1e-9), 20 * std::log10 (b + 1e-9));
        line (a > b * 3, buf);
    }
    {
        // tambour : chaque touche de l'octave a son jouet et s'arrête seul
        int ok = 0; std::string bad;
        for (int k = 0; k < 12; ++k)
        {
            auto L = play (dry (wd::Inst::Drum), 60 + k, 0.05, 2.5);
            const double e0 = rms (L, 0, 0.3), e1 = rms (L, 2.2, 2.5);
            if (e0 > 0.005 && e1 < e0 * 0.003) ++ok; else bad += " " + std::to_string (k);
        }
        std::snprintf (buf, sizeof buf, "tambour : %d sons sur 12 jouent et s'arrêtent d'eux-mêmes%s", ok, bad.empty() ? "" : (" (à revoir :" + bad + ")").c_str());
        line (ok == 12, buf);
    }
    {
        // Méca : bruit mécanique ajouté à l'attaque
        auto a = dry (wd::Inst::MusicBox), b = a; b.meca = 1;
        auto x = play (a, 72, 0.5, 0.5), y = play (b, 72, 0.5, 0.5);
        double hx = 0, hy = 0; for (int i = 1; i < 2400; ++i) { hx += std::pow (x[(size_t) i] - x[(size_t) i - 1], 2); hy += std::pow (y[(size_t) i] - y[(size_t) i - 1], 2); }
        std::snprintf (buf, sizeof buf, "Méca : l'attaque gagne un clic mécanique (énergie aiguë × %.1f)", hy / (hx + 1e-12));
        line (hy > hx * 1.5, buf);
    }

    rep += "\n## CPU (48 kHz, blocs de 512)\n\n";
    {
        struct C { const char* n; wd::Inst i; };
        const C cs[] = { { "16 voix de clochettes + écho + espace + méca + tic-tac", wd::Inst::Bells }, { "16 voix de fanfare + écho + espace", wd::Inst::Fanfare } };
        for (auto& c : cs)
        {
            auto q = dry (c.i); q.space = 0.6f; q.echo = 0.5f; q.meca = 0.8f; q.tickOn = true; q.wobble = 0.4f; q.springOn = true;
            wd::WindupEngine e; e.prepare (SR, 512); e.setParams (q); e.setTransport (120, -1);
            std::vector<float> L (512), R (512);
            for (int k = 0; k < 16; ++k) e.noteOn (48 + k * 2, 0.8f);
            const auto t0 = std::chrono::steady_clock::now();
            for (int k = 0; k < (int) (SR * 10 / 512); ++k) e.render (L.data(), R.data(), 512);
            const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::snprintf (buf, sizeof buf, "%s : %.1f %% d'un cœur", c.n, s * 10.0);
            line (s / 10 < 0.15, buf);
        }
    }
    rep += failures ? "\n**Résultat : " + std::to_string (failures) + " échec(s).**\n" : "\n**Résultat : tout est OK.**\n";
    std::fputs (rep.c_str(), stdout);
    if (argc > 1) if (auto* f = std::fopen (argv[1], "wb")) { std::fputs (rep.c_str(), f); std::fclose (f); }
    return failures ? 1 : 0;
}
