// Mesure chaque preset à -12 dB de niveau et écrit tools/levels.json (cible : -18 dB RMS sur la phrase de test).
#include "EngineCommon.h"
#include <cstdio>
int main (int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : "tools/levels.json";
    FILE* f = std::fopen (out, "w"); if (! f) return 1;
    std::fprintf (f, "{\n");
    for (int i = 0; i < kNumPresets; ++i)
    {
        auto m = renderPhrase (kPresets[i], -12.f);
        kal::SynthParams sp; kPresets[i].apply (sp);
        const double target = sp.aS < 0.1f ? -22.0 : -18.0;     // sons percussifs : RMS plus bas pour un même ressenti
        double lvl = -12.0 + (target - m.rmsDb);
        if (m.peak * std::pow (10.0, (lvl + 12) / 20) > 0.7) lvl = -12.0 + 20 * std::log10 (0.7 / m.peak);   // garde -3 dBFS de marge
        lvl = std::clamp (lvl, -30.0, 24.0);
        std::fprintf (f, "  \"%s\": %.2f%s\n", kPresets[i].id, lvl, i + 1 < kNumPresets ? "," : "");
        std::printf ("%-12s %-24s rms %6.1f dB  crête %.2f -> niveau %.1f dB\n", kPresets[i].id, kPresets[i].name, m.rmsDb, m.peak, lvl);
    }
    std::fprintf (f, "}\n"); std::fclose (f);
}
