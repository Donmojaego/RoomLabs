// =============================================================================
// Ma petite soupe ouselem — test du moteur (sans JUCE).
//   g++ -O2 -std=c++20 -Isource -Itests tests/EngineTest.cpp source/engine/SoupEngine.cpp -o engine-test && ./engine-test [RAPPORT.md]
// =============================================================================
#include "engine/SoupEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct P { const char* name; int root; float v[21]; };
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

// joue une note et renvoie le canal gauche
static std::vector<float> play (soup::Params q, int note, double holdS, double totalS, double bpm = 120)
{
    soup::SoupEngine e; e.prepare (SR, 512); e.setParams (q); e.setTransport (bpm, -1);
    std::vector<float> L ((size_t) (totalS * SR)), R (L.size());
    const int hold = (int) (holdS * SR);
    e.noteOn (note, 0.9f);
    for (int p = 0; p < (int) L.size(); p += 256)
    {
        if (p <= hold && p + 256 > hold) e.noteOff (note);
        e.render (L.data() + p, R.data() + p, std::min (256, (int) L.size() - p));
    }
    return L;
}

int main (int argc, char** argv)
{
    std::string rep = "# Ma petite soupe ouselem — rapport des tests du moteur\n\n";
    int failures = 0; char buf[600];
    auto line = [&] (bool ok, const std::string& s) { if (! ok) ++failures; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; };

    rep += "## Recettes d'usine (note de référence, 120 BPM)\n\n| Recette | Note | Justesse à sec (cents) | Crête (dBFS) | Résultat |\n|---|---|---|---|---|\n";
    int bad = 0;
    for (auto& p : presets)
    {
        auto q = toParams (p.v);
        soup::SoupEngine e; e.prepare (SR, 512); e.setParams (q); e.setTransport (120, -1);
        const double total = 1.5 + q.releaseMs / 1000.0 * 1.3 + 3.0;
        std::vector<float> L ((size_t) (total * SR)), R (L.size());
        e.noteOn (p.root, 0.9f);
        bool finite = true; double pk = 0, sum = 0;
        for (int pos = 0; pos < (int) L.size(); pos += 256)
        {
            if (pos <= (int) (1.5 * SR) && pos + 256 > (int) (1.5 * SR)) e.noteOff (p.root);
            const int n = std::min (256, (int) L.size() - pos);
            e.render (L.data() + pos, R.data() + pos, n);
            for (int i = pos; i < pos + n; ++i) { finite &= std::isfinite (L[(size_t) i]); pk = std::max ({ pk, (double) std::abs (L[(size_t) i]), (double) std::abs (R[(size_t) i]) }); sum += L[(size_t) i] * L[(size_t) i]; }
        }
        // justesse à sec : sans effet ni réverbération, instruments accordés seulement
        std::string cents = "—"; bool pitchOk = true;
        const bool tuned = ! q.arpOn && (q.inst == soup::Inst::Pad || q.inst == soup::Inst::Keys || q.inst == soup::Inst::Strings || q.inst == soup::Inst::Bass || q.inst == soup::Inst::Violin);   // cloche, texture, tambour : sons inharmoniques ou bruités
        if (tuned)
        {
            auto d = q; d.fx = soup::Fx::None; d.space = 0; d.reverseOn = d.stutterOn = false; d.attackMs = 5;
            auto x = play (d, p.root, 0.6, 0.6);
            const double start = q.inst == soup::Inst::Violin || q.inst == soup::Inst::Strings ? 0.05 : 0.03;   // avant le vibrato
            const double f = pitchOf (x.data() + (int) (start * SR), (int) (0.12 * SR), hzOf (p.root));
            const double c = f > 0 ? 1200 * std::log2 (f / hzOf (p.root)) : 9999;
            pitchOk = std::abs (c) <= 12; std::snprintf (buf, sizeof buf, "%+.1f", c); cents = buf;
        }
        const bool ok = finite && pk <= 1.0 && sum > 1e-2 && pitchOk && e.activeVoices() == 0;
        if (! ok) ++bad;
        std::snprintf (buf, sizeof buf, "| %s | %d | %s | %.1f | %s |\n", p.name, p.root, cents.c_str(), 20 * std::log10 (pk + 1e-12),
                       ok ? "OK" : ! finite ? "NaN" : pk > 1 ? "crête" : sum <= 1e-2 ? "silence" : ! pitchOk ? "justesse" : "voix bloquée");
        rep += buf;
    }
    rep += "\n## Effets calés sur le tempo (120 BPM)\n\n";
    line (bad == 0, "toutes les recettes : son, pas de NaN, crête ≤ 0 dBFS, justesse ±12 cents, voix libérées" + std::string (bad ? " (" + std::to_string (bad) + " en échec)" : ""));

    auto base = toParams (presets[0].v); base.space = 0; base.reverseOn = base.stutterOn = false; base.arpOn = false; base.tone = 0;
    // Reverse : un coup de tambour qui décroît ressort à l'envers (montée) dans la noire suivante
    {
        auto q = base; q.inst = soup::Inst::Drum; q.fx = soup::Fx::Reverse; q.division = 1; q.mix = 1; q.brightness = 0.3f;
        auto x = play (q, 48, 0.05, 1.2);
        const double first = rms (x, 0.52, 0.65), last = rms (x, 0.85, 0.98);
        std::snprintf (buf, sizeof buf, "Reverse 1/4 : le coup revient à l'envers dans la noire suivante (début %.4f < fin %.4f)", first, last);
        line (last > first * 3 && last > 1e-3, buf);
    }
    // Stutter : les tranches de 1/16 d'un temps sont identiques
    {
        auto q = base; q.inst = soup::Inst::Texture; q.fx = soup::Fx::Stutter; q.division = 3; q.mix = 1; q.attackMs = 1; q.sustain = 1;
        auto x = play (q, 60, 2.0, 2.0);
        const int L = (int) (0.125 * SR), s0 = (int) (1.0 * SR);
        double num = 0, a = 0, b = 0;
        for (int i = (int) (0.01 * SR); i < L - (int) (0.01 * SR); ++i) { num += x[(size_t) (s0 + i)] * x[(size_t) (s0 + 2 * L + i)]; a += x[(size_t) (s0 + i)] * x[(size_t) (s0 + i)]; b += x[(size_t) (s0 + 2 * L + i)] * x[(size_t) (s0 + 2 * L + i)]; }
        const double corr = num / std::sqrt (a * b + 1e-20);
        std::snprintf (buf, sizeof buf, "Stutter 1/16 : la 3e tranche du temps répète la 1re (corrélation %.3f)", corr);
        line (corr > 0.95, buf);
    }
    // Répétitions : échos à la croche
    {
        auto q = base; q.inst = soup::Inst::Drum; q.fx = soup::Fx::Repeat; q.division = 2; q.mix = 0.5f; q.feedback = 0.6f; q.brightness = 0.1f;
        auto x = play (q, 60, 0.05, 1.6);
        const double e1 = rms (x, 0.25, 0.30), gap = rms (x, 0.40, 0.45), e2 = rms (x, 0.75, 0.80);
        std::snprintf (buf, sizeof buf, "Répétitions 1/8 (aller-retour gauche/droite) : échos à gauche à 250 et 750 ms (%.4f, %.4f), silence entre (%.4f)", e1, e2, gap);
        line (e1 > gap * 4 && e2 > gap * 2 && e2 < e1, buf);
    }
    // Découpage : coupe nette sur les pas éteints du motif
    {
        auto q = base; q.inst = soup::Inst::Pad; q.fx = soup::Fx::Slicer; q.division = 3; q.mix = 1; q.attackMs = 1; q.sustain = 1;
        auto x = play (q, 60, 2.0, 2.0);
        const double on = rms (x, 1.0 + 0.01, 1.0 + 0.11), off = rms (x, 1.125 + 0.01, 1.125 + 0.11);   // pas 8 (1) et pas 9 (0)
        std::snprintf (buf, sizeof buf, "Découpage 1/16 : pas allumé %.4f, pas éteint %.5f", on, off);
        line (on > 0.01 && off < on * 0.05, buf);
    }
    // Étirement : la matière dure plus longtemps que la note
    {
        auto q = base; q.inst = soup::Inst::Strings; q.fx = soup::Fx::Stretch; q.stretch = 0.9f; q.mix = 1; q.attackMs = 5; q.releaseMs = 30;
        auto dry = q; dry.fx = soup::Fx::None;
        auto x = play (q, 60, 0.3, 2.0), y = play (dry, 60, 0.3, 2.0);
        const double wetTail = rms (x, 0.6, 0.9), dryTail = rms (y, 0.6, 0.9);
        std::snprintf (buf, sizeof buf, "Étirement : après la note, le son étiré continue (%.4f) quand le son sec s'est tu (%.5f)", wetTail, dryTail);
        line (wetTail > 0.005 && wetTail > dryTail * 20, buf);
    }
    // Filtre modulé : la couleur bouge au rythme
    {
        auto q = base; q.inst = soup::Inst::Pad; q.fx = soup::Fx::FilterMod; q.division = 1; q.mix = 1; q.attackMs = 1; q.sustain = 1; q.brightness = 1;
        auto x = play (q, 48, 3.0, 3.0);
        double mn = 1e9, mx = 0;
        for (double t = 0.5; t < 2.5; t += 0.05) { const double r = rms (x, t, t + 0.05); mn = std::min (mn, r); mx = std::max (mx, r); }
        std::snprintf (buf, sizeof buf, "Filtre modulé 1/4 : le niveau varie de %.1f dB sur un cycle de 4 noires", 20 * std::log10 (mx / mn));
        line (mx / mn > 2.0, buf);
    }
    // Grain delay : des grains reviennent après la note
    {
        auto q = base; q.inst = soup::Inst::Bell; q.fx = soup::Fx::GrainDelay; q.division = 1; q.mix = 0.8f; q.feedback = 0.4f;
        auto x = play (q, 72, 0.05, 1.5);
        const double tail = rms (x, 0.5, 0.65);
        std::snprintf (buf, sizeof buf, "Grain Delay 1/4 : grains audibles une noire plus tard (%.4f)", tail);
        line (tail > 0.003, buf);
    }
    // Interrupteurs + arpège
    {
        auto q = base; q.inst = soup::Inst::Texture; q.fx = soup::Fx::None; q.reverseOn = true; q.stutterOn = true; q.mix = 1;
        auto x = play (q, 60, 5.0, 5.0);
        bool finite = true; for (float s : x) finite &= std::isfinite (s);
        line (finite && rms (x, 2.2, 3.8) > 0.005, "interrupteurs Reverse + Stutter ensemble : son présent, sans NaN");

        soup::SoupEngine e; e.prepare (SR, 512); auto a = toParams (presets[20].v); e.setParams (a); e.setTransport (120, -1);
        std::vector<float> L (256), R (256);
        e.noteOn (60, 0.9f); e.noteOn (64, 0.9f); e.noteOn (67, 0.9f);
        for (int k = 0; k < (int) (2 * SR / 256); ++k) e.render (L.data(), R.data(), 256);
        std::snprintf (buf, sizeof buf, "arpège du Bouillon (1/16, 120 BPM) : %d notes en 2 s (attendu 16 à 17)", e.fb.noteHits.load());
        line (e.fb.noteHits.load() >= 16 && e.fb.noteHits.load() <= 17, buf);
        // synchronisation sur la position de l'hôte : le pas d'arpège suit la position donnée
        soup::SoupEngine h; h.prepare (SR, 512); h.setParams (a);
        h.setTransport (120, 10.0); h.noteOn (60, 0.9f); h.render (L.data(), R.data(), 256);
        const int afterStart = h.fb.noteHits.load();
        h.setTransport (120, 10.0 + 256.0 / SR * 2.0); h.render (L.data(), R.data(), 256);
        line (afterStart == 1 && std::abs (h.fb.beat.load() - std::fmod ((10.0 + 512.0 / SR * 2.0) / 0.25, 1.0)) < 0.02, "suit la position de l'hôte (noires) pour caler les effets et l'arpège");
    }

    rep += "\n## CPU (48 kHz, blocs de 512)\n\n";
    {
        struct C { const char* n; soup::Inst i; soup::Fx f; int notes; };
        const C cs[] = { { "12 voix de cordes + étirement + Space", soup::Inst::Strings, soup::Fx::Stretch, 12 },
                         { "8 voix de violon + Grain Delay + Reverse + Stutter", soup::Inst::Violin, soup::Fx::GrainDelay, 8 } };
        for (auto& c : cs)
        {
            auto q = base; q.inst = c.i; q.fx = c.f; q.space = 0.5f; q.mix = 0.6f; q.reverseOn = q.stutterOn = c.f == soup::Fx::GrainDelay;
            soup::SoupEngine e; e.prepare (SR, 512); e.setParams (q);
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
