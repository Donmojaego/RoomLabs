// =============================================================================
// Shake It — test du moteur, hors DAW et sans JUCE.
//   g++ -O2 -std=c++20 -Isource tests/EngineTest.cpp source/engine/BassEngine.cpp -o engine-test
//   ./engine-test [RAPPORT.md]
// Vérifie chaque preset d'usine (son, justesse, crête, NaN), le glide, le legato,
// l'absence de clics, l'énergie envoyée à l'animation et le CPU.
// Code de sortie 0 = tout est OK.
// =============================================================================
#include "engine/BassEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct P { const char* name; int root, model; float tune, glide; int glideMode; float punch, decay, drive; int driveType;
           float output, harm, tone, reso, attack, release; };
static const P presets[] = {
#include "factory_presets.inc"
};

static shk::Params toParams (const P& p)
{
    shk::Params q;
    q.model = (shk::Model) p.model; q.tune = p.tune; q.glideMs = p.glide; q.glideLegatoOnly = p.glideMode == 1;
    q.punch = p.punch; q.decay = p.decay; q.drive = p.drive; q.driveType = (shk::DriveType) p.driveType;
    q.outputDb = p.output; q.harm = p.harm; q.tone = p.tone; q.reso = p.reso; q.attackMs = p.attack; q.releaseMs = p.release;
    return q;
}

static double hzOf (double semi) { return 440.0 * std::pow (2.0, (semi - 69.0) / 12.0); }

// fondamentale par autocorrélation normalisée, cherchée autour de la fréquence attendue
static double pitchOf (const float* x, int n, double sr, double expect)
{
    const int minLag = (int) (sr / (expect * 1.5)), maxLag = (int) (sr / (expect / 1.5));
    if (maxLag * 2 >= n) return 0.0;
    double best = -1.0; int bestLag = 0;
    std::vector<double> r ((size_t) maxLag + 2, 0.0);
    for (int lag = minLag - 1; lag <= maxLag + 1; ++lag)
    {
        double s = 0, e1 = 0, e2 = 0;
        for (int i = 0; i + lag < n; ++i) { s += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
        r[(size_t) lag] = s / std::sqrt (e1 * e2 + 1e-20);
    }
    for (int lag = minLag; lag <= maxLag; ++lag)
        if (r[(size_t) lag] > best) { best = r[(size_t) lag]; bestLag = lag; }
    // interpolation parabolique
    const double a = r[(size_t) bestLag - 1], b = r[(size_t) bestLag], c = r[(size_t) bestLag + 1];
    const double d = 0.5 * (a - c) / (a - 2 * b + c + 1e-20);
    return sr / (bestLag + d);
}

int main (int argc, char** argv)
{
    const double sr = 48000.0;
    std::string rep = "# Shake It — rapport des tests du moteur\n\n";
    int failures = 0;
    auto line = [&] (bool ok, const std::string& s) { if (! ok) ++failures; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; };
    char buf[512];

    // ---------------------------------------------------------------- presets
    rep += "## Presets d'usine\n\n| Preset | Note | Écart (cents) | Crête (dBFS) | RMS (dBFS) | Résultat |\n|---|---|---|---|---|---|\n";
    int presetFails = 0;
    double worstCents = 0, worstPeak = -200;
    for (const auto& p : presets)
    {
        shk::BassEngine e; e.prepare (sr); e.setParams (toParams (p));
        const int total = (int) (sr * 3.0), hold = (int) (sr * 1.5);
        std::vector<float> out ((size_t) total);
        e.noteOn (p.root, 0.9f);
        e.render (out.data(), nullptr, hold);
        e.noteOff (p.root);
        e.render (out.data() + hold, nullptr, total - hold);

        bool finite = true; double peak = 0, sum = 0;
        for (int i = 0; i < hold; ++i) { finite &= std::isfinite (out[(size_t) i]); peak = std::max (peak, (double) std::abs (out[(size_t) i])); sum += out[(size_t) i] * out[(size_t) i]; }
        const double rmsDb = 10 * std::log10 (sum / hold + 1e-20), peakDb = 20 * std::log10 (peak + 1e-20);

        // justesse : après la chute de hauteur, avant la fin du son
        const double tau = 0.004 + (1.0 - p.punch) * 0.035;
        // Moog-style : on attend la fin du balayage du filtre
        const int start = (int) (sr * (p.model == 2 ? 0.45 : std::max (0.04, 7 * tau)));
        const int len = (int) (sr * 0.12);
        const double expect = hzOf (p.root + p.tune);
        const double f = pitchOf (out.data() + start, len, sr, expect);
        const double cents = f > 0 ? 1200 * std::log2 (f / expect) : 9999;

        // fin propre : la queue doit être retombée
        double tail = 0; for (int i = total - (int) (sr * 0.05); i < total; ++i) tail = std::max (tail, (double) std::abs (out[(size_t) i]));
        const bool ok = finite && peak <= 1.0 && rmsDb > -40 && std::abs (cents) <= 10 && tail < 0.01;
        if (! ok) ++presetFails;
        worstCents = std::max (worstCents, std::abs (cents)); worstPeak = std::max (worstPeak, peakDb);
        std::snprintf (buf, sizeof buf, "| %s | %d | %+.1f | %.1f | %.1f | %s |\n", p.name, p.root, cents, peakDb, rmsDb,
                       ok ? "OK" : (! finite ? "NaN" : peak > 1 ? "crête" : rmsDb <= -40 ? "trop faible" : tail >= 0.01 ? "queue" : "justesse"));
        rep += buf;
    }
    std::snprintf (buf, sizeof buf, "\n%d presets, pire écart %.1f cents, crête max %.1f dBFS", (int) (sizeof presets / sizeof presets[0]), worstCents, worstPeak);
    rep += buf; rep += "\n\n## Comportement\n\n";
    line (presetFails == 0, "tous les presets : son présent, pas de NaN, crête ≤ 0 dBFS, justesse ±10 cents, queue retombée" + std::string (presetFails ? " (" + std::to_string (presetFails) + " en échec)" : ""));

    // ---------------------------------------------------------------- glide
    {
        shk::BassEngine e; e.prepare (sr);
        auto q = toParams (presets[0]); q.glideMs = 300; q.glideLegatoOnly = true; q.punch = 0; e.setParams (q);
        std::vector<float> b (48000);
        e.noteOn (29, 1); e.render (b.data(), nullptr, 4800);
        e.noteOn (41, 1);                                  // legato vers l'octave
        e.render (b.data(), nullptr, (int) (sr * 0.150));
        const double mid = 12 * std::log2 (e.currentFrequency() / hzOf (29));
        e.render (b.data(), nullptr, (int) (sr * 0.160));
        const double end = 12 * std::log2 (e.currentFrequency() / hzOf (29));
        std::snprintf (buf, sizeof buf, "glide 300 ms : %.2f demi-tons à mi-chemin (attendu 6), %.2f à la fin (attendu 12)", mid, end);
        line (std::abs (mid - 6) < 0.7 && std::abs (end - 12) < 0.05, buf);

        const int before = e.motion.onsets.load();
        e.noteOff (41); e.render (b.data(), nullptr, (int) (sr * 0.4));
        const double back = 12 * std::log2 (e.currentFrequency() / hzOf (29));
        std::snprintf (buf, sizeof buf, "legato : retour à la note tenue au relâchement (%.2f demi-tons), aucune nouvelle attaque", back);
        line (std::abs (back) < 0.05 && e.motion.onsets.load() == before, buf);
        line (before == 1, "legato : une seule attaque pour deux notes liées");
    }

    // ---------------------------------------------------------------- clics
    {
        shk::BassEngine e; e.prepare (sr);
        auto q = toParams (presets[0]); q.punch = 0; q.attackMs = 1; q.releaseMs = 30; e.setParams (q);
        std::vector<float> b (24000);
        e.noteOn (29, 1); e.render (b.data(), nullptr, 2400);
        double firstJump = std::abs (b[0]);
        e.noteOff (29); e.render (b.data(), nullptr, 24000);
        double maxDiff = 0; for (int i = 1; i < 24000; ++i) maxDiff = std::max (maxDiff, (double) std::abs (b[(size_t) i] - b[(size_t) i - 1]));
        std::snprintf (buf, sizeof buf, "pas de clic : premier échantillon %.4f, saut max au relâchement %.4f", firstJump, maxDiff);
        line (firstJump < 0.01 && maxDiff < 0.05, buf);

        // nouvelle note pendant la queue : repart du niveau actuel
        e.noteOn (29, 1); e.render (b.data(), nullptr, 4800); e.noteOff (29); e.render (b.data(), nullptr, 480);
        const float lastBefore = b[479];
        e.noteOn (31, 1); e.render (b.data(), nullptr, 480);
        std::snprintf (buf, sizeof buf, "nouvelle note pendant la queue : saut %.4f", std::abs (b[0] - lastBefore));
        line (std::abs (b[0] - lastBefore) < 0.08, buf);
    }

    // ---------------------------------------------------------------- animation
    {
        shk::BassEngine e; e.prepare (sr); e.setParams (toParams (presets[0]));
        std::vector<float> b (480);
        e.noteOn (29, 1);
        float maxE = 0; for (int k = 0; k < 50; ++k) { e.render (b.data(), nullptr, 480); maxE = std::max (maxE, e.motion.energy.load()); }
        e.noteOff (29);
        for (int k = 0; k < 300; ++k) e.render (b.data(), nullptr, 480);
        const float rest = e.motion.energy.load();
        std::snprintf (buf, sizeof buf, "énergie du grave : %.2f en jeu, %.4f après 3 s de silence, compteur d'attaques %d, note au repos %d",
                       maxE, rest, e.motion.onsets.load(), e.motion.note.load());
        line (maxE > 0.3f && rest < 0.01f && e.motion.onsets.load() == 1 && e.motion.note.load() == -1, buf);
    }

    // ---------------------------------------------------------------- CPU
    rep += "\n## CPU (une instance, 48 kHz, note tenue)\n\n";
    for (int m = 0; m < 4; ++m)
    {
        static const char* names[] = { "808", "Sub", "Moog-style", "Pincée" };
        shk::BassEngine e; e.prepare (sr);
        auto q = toParams (presets[0]); q.model = (shk::Model) m; q.decay = 8; q.drive = 0.5f; q.reso = 0.5f; e.setParams (q);
        std::vector<float> l (512), r (512);
        const int blocks = (int) (sr * 10 / 512);
        e.noteOn (36, 1);
        const auto t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < blocks; ++k) e.render (l.data(), r.data(), 512);
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::snprintf (buf, sizeof buf, "%s : %.2f %% d'un cœur", names[m], 100.0 * s / 10.0);
        line (s / 10.0 < 0.03, buf);
    }

    rep += failures ? "\n**Résultat : " + std::to_string (failures) + " échec(s).**\n" : "\n**Résultat : tout est OK.**\n";
    std::fputs (rep.c_str(), stdout);
    if (argc > 1) if (auto* fp = std::fopen (argv[1], "wb")) { std::fputs (rep.c_str(), fp); std::fclose (fp); }
    return failures ? 1 : 0;
}
