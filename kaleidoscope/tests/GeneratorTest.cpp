// Kaleidoscope — test du générateur (sans JUCE)
//   g++ -O2 -std=c++20 -Isource tests/GeneratorTest.cpp source/engine/Generator.cpp -o gen-test && ./gen-test [RAPPORT.md]
#include "engine/Generator.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

using namespace kal;
int main (int argc, char** argv)
{
    std::string rep = "# Kaleidoscope — rapport des tests du générateur\n\n"; int failures = 0; char buf[500];
    auto line = [&] (bool ok, const std::string& s) { if (! ok) ++failures; rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s + "\n"; };
    Generator g;
    // 1) chaque style x chaque mode x quelques humeurs : gamme respectée, solo monophonique, accompagnement en accords
    int total = 0, outScale = 0, overlaps = 0, accompNoChord = 0, empty = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int s = 0; s < Generator::numStyles(); ++s)
        for (int m = 0; m < 2; ++m)
            for (int md = 0; md < Generator::numMoods(); md += 3)
            {
                Request r; r.style = s; r.mood = md; r.mode = (Mode) m; r.root = (s * 5) % 12; r.bars = 4;
                Pattern p = g.generate (r); g.remember (p); ++total;
                if (p.notes.empty()) { ++empty; continue; }
                const auto& S = Generator::scaleDef (p.scale);
                for (auto& n : p.notes) outScale += ! inScale (S, p.root, n.pitch);
                if (m == 0) { for (size_t i = 0; i + 1 < p.notes.size(); ++i) overlaps += p.notes[i].start + p.notes[i].len > p.notes[i + 1].start + 1e-6; }
                else { int chordsFound = 0; for (size_t i = 0; i + 2 < p.notes.size(); ++i) chordsFound += std::abs (p.notes[i].start - p.notes[i + 2].start) < 1.0; accompNoChord += chordsFound == 0; }
            }
    const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count() / total;
    std::snprintf (buf, sizeof buf, "%d propositions (%d styles x Solo/Accompagnement x 4 humeurs) : %d vides, %d notes hors gamme", total, Generator::numStyles(), empty, outScale);
    line (empty == 0 && outScale == 0, buf);
    std::snprintf (buf, sizeof buf, "Solo monophonique : %d chevauchements", overlaps); line (overlaps == 0, buf);
    std::snprintf (buf, sizeof buf, "Accompagnement : %d propositions sans accord (attendu 0)", accompNoChord); line (accompNoChord == 0, buf);
    std::snprintf (buf, sizeof buf, "temps de génération : %.1f ms par clic (24 candidates)", ms); line (ms < 60, buf);

    // 2) Keys : gamme imposée
    {
        int bad = 0;
        for (int sc = 0; sc < Generator::numScales(); ++sc) { Request r; r.style = 9; r.scale = sc; r.root = 7; Pattern p = g.generate (r); for (auto& n : p.notes) bad += ! inScale (Generator::scaleDef (sc), 7, n.pitch); bad += p.scale != sc; }
        std::snprintf (buf, sizeof buf, "Keys : les %d gammes imposées (sol) sont respectées, %d écarts", Generator::numScales(), bad); line (bad == 0, buf);
    }
    // 3) graine : même graine = même proposition
    {
        Request r; r.style = 0; r.mood = 1; Pattern a = g.generate (r); r.seed = a.seed; Pattern b = g.generate (r);
        bool same = a.notes.size() == b.notes.size(); for (size_t i = 0; same && i < a.notes.size(); ++i) same = a.notes[i].pitch == b.notes[i].pitch && a.notes[i].start == b.notes[i].start;
        std::snprintf (buf, sizeof buf, "graine %016llx : la même graine redonne la même mélodie", (unsigned long long) a.seed); line (same, buf);
    }
    // 4) anti-répétition : 60 clics d'affilée dans le même style
    {
        Generator h; std::vector<Pattern> ps; double maxSim = 0, sum = 0; int pairs = 0;
        for (int i = 0; i < 60; ++i) { Request r; r.style = 13; r.mood = 4; Pattern p = h.generate (r); h.remember (p); ps.push_back (p); }
        for (size_t i = 0; i < ps.size(); ++i) for (size_t j = i + 1; j < ps.size(); ++j) { const double s = Generator::similarity (ps[i], ps[j]); maxSim = std::max (maxSim, s); sum += s; ++pairs; }
        std::snprintf (buf, sizeof buf, "60 clics (Afro love, romantique) : ressemblance moyenne %.2f, maximale %.2f (doublons refusés au-delà de 0,60)", sum / pairs, maxSim);
        line (maxSim <= 0.6 && sum / pairs < 0.2, buf);
    }
    // 5) Déclinaison : liée à la référence, plus ou moins selon la force
    {
        Request r; r.style = 9; r.mood = 4; Pattern ref = g.generate (r);
        double low = 0, high = 0, other = 0;
        for (int i = 0; i < 10; ++i) { r.variation = 0.2f; low += Generator::similarity (g.decline (ref, r), ref); r.variation = 0.9f; high += Generator::similarity (g.decline (ref, r), ref); other += Generator::similarity (g.generate (r), ref); }
        std::snprintf (buf, sizeof buf, "Déclinaison : ressemblance à la référence %.2f (variation 20 %%), %.2f (90 %%), contre %.2f pour une nouvelle idée", low / 10, high / 10, other / 10);
        line (low > high && high > other + 0.1, buf);
    }
    // 6) Personal : après des « J'aime » sur des mélodies denses, les propositions deviennent plus denses
    {
        Generator h; Profile prof; Request r; r.style = 50; r.mood = 0; r.personal = 1.f;
        int liked = 0; for (int i = 0; i < 300 && liked < 12; ++i) { Pattern p = h.generate (r); if (p.feat.density > 0.25f) { prof.like (p.feat); ++liked; } }
        double before = 0, after = 0, dB = 0, dA = 0;
        for (int i = 0; i < 30; ++i) { Pattern a = h.generate (r); before += prof.score (a.feat); dB += a.feat.density; Pattern b = h.generate (r, &prof); after += prof.score (b.feat); dA += b.feat.density; }
        std::snprintf (buf, sizeof buf, "Personal : après 12 « J'aime » sur des mélodies denses, proximité aux goûts %.2f -> %.2f (densité %.2f -> %.2f)", before / 30, after / 30, dB / 30, dA / 30);
        line (after > before + 0.2, buf);
        Profile back; back.fromString (prof.toString());
        line (back.nPos == prof.nPos && std::abs (back.pos[0] - prof.pos[0]) < 1e-6, "Personal : le profil se sauvegarde et se relit à l'identique");
    }
    rep += failures ? "\n**Résultat : " + std::to_string (failures) + " échec(s).**\n" : "\n**Résultat : tout est OK.**\n";
    std::fputs (rep.c_str(), stdout);
    if (argc > 1) if (auto* f = std::fopen (argv[1], "wb")) { std::fputs (rep.c_str(), f); std::fclose (f); }
    return failures ? 1 : 0;
}
