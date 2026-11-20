// =============================================================================
// Candy Shop — test du moteur, hors DAW et sans JUCE.
//   g++ -O2 -std=c++20 -Isource -Itests tests/EngineTest.cpp source/engine/CandyEngine.cpp -o engine-test
//   ./engine-test [RAPPORT.md]
// =============================================================================
#include "engine/CandyEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct P { const char* name; int root; float v[31]; };
static const P presets[] = {
#include "factory_presets.inc"
};

static cdy::Params toParams (const P& p)
{
    const float* v = p.v; cdy::Params q;
    q.algo = (cdy::Algo) (int) v[0]; q.wave = (int) v[1]; q.detune = v[2]; q.unison = (int) v[3];
    q.fmRatio = v[4]; q.fmAmount = v[5]; q.fmDecay = v[6]; q.noise = v[7]; q.pitchDrop = v[8]; q.tune = v[9];
    q.attackMs = v[10]; q.decayMs = v[11]; q.sustain = v[12]; q.releaseMs = v[13]; q.tone = v[14]; q.cutoff = v[15];
    q.reso = v[16]; q.fenv = v[17]; q.fdecayMs = v[18]; q.space = v[19]; q.chorus = v[20]; q.levelDb = v[21];
    q.velSens = v[22]; q.chord = (int) v[23]; q.arpOn = v[24] > 0.5f; q.arpRate = (int) v[25]; q.arpMode = (int) v[26];
    q.arpOct = (int) v[27]; q.arpGate = v[28]; q.kit = (int) v[29]; q.drumLevelDb = v[30];
    return q;
}

static double hzOf (double semi) { return 440.0 * std::pow (2.0, (semi - 69.0) / 12.0); }

static double pitchOf (const float* x, int n, double sr, double expect)
{
    const int minLag = (int) (sr / (expect * 1.4)), maxLag = (int) (sr / (expect / 1.4));
    if (maxLag + 2 >= n) return 0.0;
    std::vector<double> r ((size_t) maxLag + 3, 0.0);
    for (int lag = minLag - 1; lag <= maxLag + 1; ++lag)
    {
        double s = 0, e1 = 0, e2 = 0;
        for (int i = 0; i + lag < n; ++i) { s += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
        r[(size_t) lag] = s / std::sqrt (e1 * e2 + 1e-20);
    }
    int best = minLag;
    for (int lag = minLag; lag <= maxLag; ++lag) if (r[(size_t) lag] > r[(size_t) best]) best = lag;
    const double a = r[(size_t) best - 1], b = r[(size_t) best], c = r[(size_t) best + 1];
    return sr / (best + 0.5 * (a - c) / (a - 2 * b + c + 1e-20));
}

int main (int argc, char** argv)
{
    const double sr = 48000.0;
    std::string rep = "# Candy Shop — rapport des tests du moteur\n\n";
    int failures = 0; char buf[512];
    auto line = [&] (bool ok, const std::string& s) { if (! ok) ++failures; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; };

    // ---------------------------------------------------------------- presets
    rep += "## Presets d'usine (note de référence tenue 1 s, tempo 120)\n\n| Preset | Note | Justesse (cents) | Crête (dBFS) | Notes jouées | Résultat |\n|---|---|---|---|---|---|\n";
    int bad = 0; double worstCents = 0;
    for (const auto& p : presets)
    {
        cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (p); e.setParams (q); e.setTempo (120);
        const int hold = (int) (sr * 1.0), total = hold + (int) (sr * (q.releaseMs * 0.0012 + 1.0));
        std::vector<float> L ((size_t) total), R ((size_t) total);
        e.noteOn (1, p.root, 0.9f);
        e.render (L.data(), R.data(), hold);
        e.noteOff (1, p.root);
        e.render (L.data() + hold, R.data() + hold, total - hold);

        bool finite = true; double peak = 0, sum = 0;
        for (int i = 0; i < total; ++i) { finite &= std::isfinite (L[(size_t) i]) && std::isfinite (R[(size_t) i]);
            peak = std::max ({ peak, (double) std::abs (L[(size_t) i]), (double) std::abs (R[(size_t) i]) }); sum += L[(size_t) i] * L[(size_t) i]; }

        // justesse : notes seules (ni accord, ni arpège), modèles harmoniques ; sans réverbération pour la mesure
        std::string cents = "—"; bool pitchOk = true;
        if (q.chord == 0 && ! q.arpOn && q.algo != cdy::Algo::Fm)
        {
            cdy::CandyEngine d; auto q2 = q; q2.space = 0; q2.chorus = 0; d.prepare (sr, 512); d.setParams (q2);
            std::vector<float> x ((size_t) (sr * 0.4)), y ((size_t) (sr * 0.4));
            d.noteOn (1, p.root, 0.9f); d.render (x.data(), y.data(), (int) x.size());
            const int start = (int) (sr * (q.pitchDrop > 0 ? 0.16 : (q.attackMs > 200 ? 0.25 : 0.03)));
            const int len = std::min ((int) (sr * 0.06), (int) x.size() - start);
            const double f = pitchOf (x.data() + start, len, sr, hzOf (p.root + q.tune));
            const double c = f > 0 ? 1200 * std::log2 (f / hzOf (p.root + q.tune)) : 9999;
            pitchOk = std::abs (c) <= 10; worstCents = std::max (worstCents, std::abs (c));
            std::snprintf (buf, sizeof buf, "%+.1f", c); cents = buf;
        }
        const int hits = e.fb.noteHits.load();
        const bool arpOk = ! q.arpOn || hits >= 3;
        const bool ok = finite && peak <= 1.0 && sum > 1e-2 && pitchOk && arpOk && e.activeVoices() == 0;
        if (! ok) ++bad;
        std::snprintf (buf, sizeof buf, "| %s | %d | %s | %.1f | %d | %s |\n", p.name, p.root, cents.c_str(), 20 * std::log10 (peak + 1e-12), hits,
                       ok ? "OK" : ! finite ? "NaN" : peak > 1 ? "crête" : sum <= 1e-2 ? "silence" : ! pitchOk ? "justesse" : ! arpOk ? "arpège" : "voix bloquée");
        rep += buf;
    }
    std::snprintf (buf, sizeof buf, "\n%d presets, pire écart de justesse %.1f cents\n\n## Comportement\n\n", (int) (sizeof presets / sizeof presets[0]), worstCents);
    rep += buf;
    line (bad == 0, "tous les presets : son présent, pas de NaN, crête ≤ 0 dBFS, justesse ±10 cents, voix libérées" + std::string (bad ? " (" + std::to_string (bad) + " en échec)" : ""));

    // ---------------------------------------------------------------- boîte à rythmes
    {
        int silent = 0; double worst = 0;
        for (int kit = 0; kit < 5; ++kit)
            for (int slot = 0; slot < 12; ++slot)
            {
                cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (presets[0]); q.kit = kit; q.space = 0; e.setParams (q);
                std::vector<float> L ((size_t) sr * 4), R ((size_t) sr * 4);
                e.noteOn (1, cdy::kDrumLow + slot, 1.f);
                e.render (L.data(), R.data(), (int) L.size());
                double pk = 0, tail = 0;
                for (size_t i = 0; i < L.size(); ++i) { pk = std::max (pk, (double) std::abs (L[i])); if (std::isnan (L[i])) pk = 9; }
                for (size_t i = L.size() - 4800; i < L.size(); ++i) tail = std::max (tail, (double) std::abs (L[i]));
                if (pk < 0.02 || pk > 1.0 || tail > 1e-3) { ++silent; std::snprintf (buf, sizeof buf, "  (kit %d, son %d : crête %.3f, fin %.5f)", kit, slot, pk, tail); rep += std::string (buf) + "\n"; }
                worst = std::max (worst, pk);
            }
        std::snprintf (buf, sizeof buf, "boîte à rythmes : 12 sons × 5 kits audibles, sans NaN, crête max %.1f dBFS, fin propre", 20 * std::log10 (worst));
        line (silent == 0, buf + std::string (silent ? " (" + std::to_string (silent) + " en échec)" : ""));

        cdy::CandyEngine e; e.prepare (sr, 512); e.setParams (toParams (presets[0]));
        std::vector<float> L (480), R (480);
        e.noteOn (10, 60, 1.f);               // canal 10 : percussion, pas de note d'instrument
        e.render (L.data(), R.data(), 480);
        line (e.fb.noteHits.load() == 0 && e.fb.drumHits[0].load() + e.fb.drumHits[1].load() + e.fb.drumHits[2].load() == 1,
              "canal MIDI 10 et notes 24 à 35 : boîte à rythmes, sans déclencher l'instrument");
    }

    // ---------------------------------------------------------------- accords
    {
        cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (presets[0]); q.algo = cdy::Algo::Synth; q.chord = 3; q.sustain = 0.8f; e.setParams (q);
        std::vector<float> L (4800), R (4800);
        e.noteOn (1, 60, 1.f); e.render (L.data(), R.data(), 4800);
        std::snprintf (buf, sizeof buf, "accord maj7 : %d voix pour une touche", e.activeVoices());
        line (e.activeVoices() == 4, buf);
        e.noteOff (1, 60); for (int k = 0; k < 40; ++k) e.render (L.data(), R.data(), 4800);
        line (e.activeVoices() == 0, "accord : toutes les voix se libèrent au relâchement");
    }

    // ---------------------------------------------------------------- arpège
    {
        cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (presets[0]); q.algo = cdy::Algo::Synth; q.sustain = 0.8f;
        q.arpOn = true; q.arpRate = 3; q.arpMode = 0; q.arpOct = 1; e.setParams (q); e.setTempo (120);
        std::vector<float> L (480), R (480);
        e.noteOn (1, 60, 1.f); e.noteOn (1, 64, 1.f); e.noteOn (1, 67, 1.f);
        for (int k = 0; k < 200; ++k) e.render (L.data(), R.data(), 480);   // 2 s
        const int steps = e.fb.arpStep.load();
        std::snprintf (buf, sizeof buf, "arpège 1/16 à 120 BPM : %d pas en 2 s (attendu 16 à 17)", steps);
        line (steps >= 16 && steps <= 17, buf);
        e.setTempo (90);
        const int before = e.fb.arpStep.load();
        for (int k = 0; k < 200; ++k) e.render (L.data(), R.data(), 480);
        std::snprintf (buf, sizeof buf, "arpège suit le tempo : %d pas en 2 s à 90 BPM (attendu 12)", e.fb.arpStep.load() - before);
        line (std::abs (e.fb.arpStep.load() - before - 12) <= 1, buf);
        e.noteOff (1, 60); e.noteOff (1, 64); e.noteOff (1, 67);
        for (int k = 0; k < 200; ++k) e.render (L.data(), R.data(), 480);
        line (e.activeVoices() == 0 && e.fb.arpStep.load() - before <= 13, "arpège : s'arrête au relâchement des touches");
    }

    // ---------------------------------------------------------------- polyphonie
    {
        cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (presets[30]); e.setParams (q);   // pad
        std::vector<float> L (480), R (480);
        bool finite = true;
        for (int n = 0; n < 24; ++n) { e.noteOn (1, 48 + n, 0.8f); e.render (L.data(), R.data(), 480); for (float x : L) finite &= std::isfinite (x); }
        std::snprintf (buf, sizeof buf, "24 notes tenues : %d voix (maximum 16), pas de NaN", e.activeVoices());
        line (e.activeVoices() <= 16 && finite, buf);
        e.allNotesOff(); for (int k = 0; k < 600; ++k) e.render (L.data(), R.data(), 480);
        line (e.activeVoices() == 0, "toutes les notes coupées : silence après le relâchement");
    }

    // ---------------------------------------------------------------- CPU
    rep += "\n## CPU (48 kHz, blocs de 512)\n\n";
    {
        struct Case { const char* name; int preset; int notes; };
        const Case cases[] = { { "16 voix de pad + chorus + Space", 30, 16 }, { "marimba, 6 notes", 10, 6 }, { "arpège + accords", 70, 3 } };
        for (auto& c : cases)
        {
            cdy::CandyEngine e; e.prepare (sr, 512); auto q = toParams (presets[c.preset]); q.space = 0.6f; q.chorus = 0.5f; e.setParams (q);
            std::vector<float> L (512), R (512);
            for (int n = 0; n < c.notes; ++n) e.noteOn (1, 48 + n * 3, 0.8f);
            for (int s = 0; s < 4; ++s) e.noteOn (1, cdy::kDrumLow + s, 1.f);
            const int blocks = (int) (sr * 10 / 512);
            const auto t0 = std::chrono::steady_clock::now();
            for (int k = 0; k < blocks; ++k) { e.render (L.data(), R.data(), 512); if (k % 40 == 0) e.noteOn (1, cdy::kDrumLow + k % 12, 1.f); }
            const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::snprintf (buf, sizeof buf, "%s : %.1f %% d'un cœur", c.name, 100.0 * s / 10.0);
            line (s / 10.0 < 0.15, buf);
        }
    }

    rep += failures ? "\n**Résultat : " + std::to_string (failures) + " échec(s).**\n" : "\n**Résultat : tout est OK.**\n";
    std::fputs (rep.c_str(), stdout);
    if (argc > 1) if (auto* fp = std::fopen (argv[1], "wb")) { std::fputs (rep.c_str(), fp); std::fclose (fp); }
    return failures ? 1 : 0;
}
