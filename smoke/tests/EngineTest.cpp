// =============================================================================
// Smoke — test du moteur (sans JUCE).
//   g++ -O2 -std=c++20 -Isource -Itests tests/EngineTest.cpp source/engine/SmokeEngine.cpp -o engine-test && ./engine-test [RAPPORT.md]
// =============================================================================
#include "engine/SmokeEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct P { const char* name; int root; float v[17]; };
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
static std::vector<float> play (smk::Params q, int note, double holdS, double totalS, float vel = 0.9f, smk::SmokeEngine* keep = nullptr)
{
    smk::SmokeEngine local; auto& e = keep ? *keep : local;
    e.prepare (SR, 512); e.setParams (q);
    std::vector<float> L ((size_t) (totalS * SR)), R (L.size());
    const int hold = (int) (holdS * SR);
    e.noteOn (note, vel);
    for (int p = 0; p < (int) L.size(); p += 256)
    {
        if (p <= hold && p + 256 > hold) e.noteOff (note);
        e.render (L.data() + p, R.data() + p, std::min (256, (int) L.size() - p));
    }
    return L;
}
static smk::Params dry (smk::Model m)
{
    smk::Params q; q.model = m; q.space = 0; q.dream = 0; q.mod = 0; q.clean = 1; q.tone = 1; q.levelDb = -6; q.chord = 0;
    q.sustain = 0.8f; q.decayMs = 3000; q.releaseMs = 300; q.riseS = 4; return q;
}

int main (int argc, char** argv)
{
    std::string rep = "# Smoke — rapport des tests du moteur\n\n";
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
        const bool ok = finite && pkDb > -12 && pkDb < -1 && tail < -50;
        if (! ok) ++failures;
        std::snprintf (buf, sizeof buf, "| %s | %d | %.1f | %.0f dB | %s |\n", presets[i].name, presets[i].root, pkDb, tail, ok ? "OK" : "ÉCHEC");
        rep += buf;
    }

    rep += "\n## Justesse (à sec, sans dérive)\n\n";
    {
        struct T { const char* n; smk::Model m; int note; };
        const T ts[] = { { "Rhodes", smk::Model::Rhodes, 60 }, { "Piano feutré", smk::Model::Felt, 60 }, { "Nappe", smk::Model::Pad, 57 },
                         { "Sub", smk::Model::Sub, 36 }, { "Basse house", smk::Model::HouseBass, 36 } };
        for (auto& t : ts)
        {
            auto L = play (dry (t.m), t.note, 1.5, 1.5);
            const int off = (int) (0.4 * SR), n = (int) (0.4 * SR);
            const double hz = pitchOf (L.data() + off, n, hzOf (t.note)), cents = 1200 * std::log2 (hz / hzOf (t.note));
            std::snprintf (buf, sizeof buf, "%s (note %d) : %+.1f cents", t.n, t.note, cents);
            line (std::abs (cents) < 8, buf);
        }
        auto q = dry (smk::Model::Riser); q.riseS = 2;
        auto L = play (q, 60, 3.0, 3.0);
        const double h0 = pitchOf (L.data() + (int) (0.1 * SR), (int) (0.15 * SR), hzOf (60)), h1 = pitchOf (L.data() + (int) (2.4 * SR), (int) (0.2 * SR), hzOf (67));
        std::snprintf (buf, sizeof buf, "Riser : monte de %.0f Hz à %.0f Hz en 2 s (une quinte attendue, %.0f → %.0f)", h0, h1, hzOf (60), hzOf (67));
        line (std::abs (1200 * std::log2 (h0 / hzOf (60))) < 30 && std::abs (1200 * std::log2 (h1 / hzOf (67))) < 30, buf);
    }

    rep += "\n## Modules\n\n";
    {
        auto base = dry (smk::Model::Rhodes);
        auto a = play (base, 60, 0.3, 3.0); auto qb = base; qb.space = 0.8f; qb.dream = 1; auto b = play (qb, 60, 0.3, 3.0);
        const double ta = rms (a, 2.0, 2.5), tb = rms (b, 2.0, 2.5);
        std::snprintf (buf, sizeof buf, "Space + Dream allongent la queue : %.1f dB → %.1f dB, 1,7 s après la relâche", 20 * std::log10 (ta + 1e-12), 20 * std::log10 (tb + 1e-12));
        line (tb > ta * 10, buf);

        auto bb = dry (smk::Model::Pad); bb.bright = 1; auto qt = bb; qt.tone = 0.05f; auto t = play (qt, 48, 1.0, 1.0);
        auto hp = [] (const std::vector<float>& x) { double s = 0, p = 0; for (size_t i = 1; i < x.size(); ++i) { const double d = x[i] - x[i - 1]; s += d * d; p += (double) x[i] * x[i]; } return std::sqrt (s / (p + 1e-20)); };
        auto open = play (bb, 48, 1.0, 1.0);
        std::snprintf (buf, sizeof buf, "Tone fermé assombrit le son (indice de brillance %.3f → %.3f)", hp (open), hp (t));
        line (hp (t) < hp (open) * 0.6, buf);

        auto qd = base; qd.clean = 0; qd.bright = 0.8f; auto d = play (qd, 48, 1.0, 1.0);
        auto qc = base; qc.bright = 0.8f; auto c = play (qc, 48, 1.0, 1.0);
        auto thd = [] (const std::vector<float>& x) { double k = 0, s = 0; for (float v : x) { s += v * v; k += v * v * v * v; } return k * x.size() / (s * s + 1e-20); };
        std::snprintf (buf, sizeof buf, "Clean à 0 compresse la forme d'onde (facteur de crête %.2f → %.2f)", thd (c), thd (d));
        line (thd (d) < thd (c), buf);

        auto qm = base; qm.mod = 1; auto m = play (qm, 60, 4.0, 4.0);
        std::vector<float> x1 (m.begin() + (int) (0.5 * SR), m.begin() + (int) (0.8 * SR)), x2 (m.begin() + (int) (2.6 * SR), m.begin() + (int) (2.9 * SR));
        const double p1 = pitchOf (x1.data(), (int) x1.size(), hzOf (60)), p2 = pitchOf (x2.data(), (int) x2.size(), hzOf (60));
        std::snprintf (buf, sizeof buf, "Mod fait dériver la hauteur lentement (%.2f Hz puis %.2f Hz, ±14 cents max)", p1, p2);
        line (std::abs (p1 - p2) > 0.05 && std::abs (1200 * std::log2 (p1 / hzOf (60))) < 20, buf);
    }

    rep += "\n## Jeu\n\n";
    {
        auto q = dry (smk::Model::Rhodes); q.chord = 1;
        smk::SmokeEngine e; e.prepare (SR, 512); e.setParams (q);
        std::vector<float> L (256), R (256);
        e.noteOn (57, 0.8f); e.render (L.data(), R.data(), 256);
        std::snprintf (buf, sizeof buf, "accord m9 : une touche lance %d voix (attendu 5)", e.activeVoices());
        line (e.activeVoices() == 5, buf);
        e.noteOff (57); for (int k = 0; k < (int) (2 * SR / 256); ++k) e.render (L.data(), R.data(), 256);
        line (e.activeVoices() == 0, "relâcher la touche libère toutes les voix de l'accord");

        auto qs = dry (smk::Model::Sub); qs.glideMs = 100;
        smk::SmokeEngine s; s.prepare (SR, 512); s.setParams (qs);
        s.noteOn (36, 0.9f); s.noteOn (43, 0.9f);
        std::vector<float> B ((size_t) SR), BR (B.size()); s.render (B.data(), BR.data(), (int) B.size());
        const double g0 = pitchOf (B.data() + (int) (0.01 * SR), (int) (0.05 * SR), hzOf (40)), g1 = pitchOf (B.data() + (int) (0.6 * SR), (int) (0.2 * SR), hzOf (43));
        std::snprintf (buf, sizeof buf, "Sub mono legato : %d voix, glisse de %.0f Hz vers %.0f Hz (cible %.0f)", s.activeVoices(), g0, g1, hzOf (43));
        line (s.activeVoices() == 1 && g0 < hzOf (43) * 0.97 && std::abs (1200 * std::log2 (g1 / hzOf (43))) < 10, buf);
        s.noteOff (43); s.render (B.data(), BR.data(), 19200);
        const double back = pitchOf (B.data() + 14400, 4800, hzOf (36));
        std::snprintf (buf, sizeof buf, "Sub : relâcher la note du haut revient à la note tenue (%.0f Hz, attendu %.0f)", back, hzOf (36));
        line (std::abs (1200 * std::log2 (back / hzOf (36))) < 60, buf);

        smk::SmokeEngine f; f.prepare (SR, 512); f.setParams (dry (smk::Model::Pad));
        for (int k = 0; k < 20; ++k) f.noteOn (40 + k, 0.7f);
        f.render (L.data(), R.data(), 256);
        std::snprintf (buf, sizeof buf, "20 touches d'un coup : %d voix au maximum (vol de voix)", f.activeVoices());
        line (f.activeVoices() == 12, buf);
    }

    rep += "\n## Retour visuel (fumée, braise, yeux)\n\n";
    {
        auto q = dry (smk::Model::Rhodes); q.space = 0.8f; q.dream = 0.8f; q.mod = 0.6f; q.clean = 0.2f;
        smk::SmokeEngine e; e.prepare (SR, 512); e.setParams (q);
        std::vector<float> L (256), R (256);
        e.noteOn (60, 1.f); for (int k = 0; k < 40; ++k) e.render (L.data(), R.data(), 256);
        const float en = e.fb.energy.load(), in = e.fb.intensity.load(); const int held = e.fb.held.load(), hits = e.fb.noteHits.load();
        std::snprintf (buf, sizeof buf, "note tenue : énergie %.2f, intensité %.2f, touches tenues %d, coups %d", en, in, held, hits);
        line (en > 0.05f && in > 0.3f && held == 1 && hits == 1, buf);
        e.noteOff (60); for (int k = 0; k < (int) (12 * SR / 256); ++k) e.render (L.data(), R.data(), 256);
        std::snprintf (buf, sizeof buf, "après la relâche et la queue : énergie %.3f, intensité %.3f, touches %d (la fumée se dissipe)", e.fb.energy.load(), e.fb.intensity.load(), e.fb.held.load());
        line (e.fb.energy.load() < 0.01f && e.fb.intensity.load() < 0.05f && e.fb.held.load() == 0, buf);
    }

    rep += "\n## CPU (48 kHz, blocs de 512)\n\n";
    {
        struct C { const char* n; smk::Model m; int notes; };
        const C cs[] = { { "12 voix de piano feutré + Space/Dream/Mod", smk::Model::Felt, 12 }, { "12 voix de nappe + Space/Dream/Mod", smk::Model::Pad, 12 } };
        for (auto& c : cs)
        {
            auto q = dry (c.m); q.space = 0.7f; q.dream = 0.8f; q.mod = 0.6f; q.clean = 0.3f;
            smk::SmokeEngine e; e.prepare (SR, 512); e.setParams (q);
            std::vector<float> L (512), R (512);
            for (int k = 0; k < c.notes; ++k) e.noteOn (48 + k * 2, 0.8f);
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
