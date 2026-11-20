// Mesure la crête de chaque preset à niveau 0 dB et écrit presets/levels.json
// (niveau qui place chaque preset vers -6 dBFS). Utilisé par generate_factory_presets.py.
//   g++ -O2 -std=c++20 -Isource -Itests tools/calibrate_levels.cpp source/engine/CandyEngine.cpp -o calibrate && ./calibrate presets/levels.json
#include "engine/CandyEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
struct P { const char* name; int root; float v[31]; };
static const P presets[] = {
#include "factory_presets.inc"
};
int main (int argc, char** argv)
{
    FILE* f = std::fopen (argc > 1 ? argv[1] : "levels.json", "wb");
    std::fputs ("{\n", f);
    const int count = (int) (sizeof presets / sizeof presets[0]);
    for (int i = 0; i < count; ++i)
    {
        const float* v = presets[i].v; cdy::Params q;
        q.algo = (cdy::Algo) (int) v[0]; q.wave = (int) v[1]; q.detune = v[2]; q.unison = (int) v[3]; q.fmRatio = v[4]; q.fmAmount = v[5];
        q.fmDecay = v[6]; q.noise = v[7]; q.pitchDrop = v[8]; q.tune = v[9]; q.attackMs = v[10]; q.decayMs = v[11]; q.sustain = v[12];
        q.releaseMs = v[13]; q.tone = v[14]; q.cutoff = v[15]; q.reso = v[16]; q.fenv = v[17]; q.fdecayMs = v[18]; q.space = v[19];
        q.chorus = v[20]; q.levelDb = 0; q.velSens = v[22]; q.chord = (int) v[23]; q.arpOn = v[24] > 0.5f; q.arpRate = (int) v[25];
        q.arpMode = (int) v[26]; q.arpOct = (int) v[27]; q.arpGate = v[28]; q.kit = (int) v[29];
        cdy::CandyEngine e; e.prepare (48000, 512); e.setParams (q); e.setTempo (120);
        std::vector<float> L (48000), R (48000);
        e.noteOn (1, presets[i].root, 0.9f); e.render (L.data(), R.data(), 48000);
        double pk = 1e-9; for (int k = 0; k < 48000; ++k) pk = std::max ({ pk, (double) std::abs (L[(size_t) k]), (double) std::abs (R[(size_t) k]) });
        const double level = std::clamp (-6.0 - 20 * std::log10 (pk), -30.0, 6.0);
        std::fprintf (f, "  \"%s\": %.1f%s\n", presets[i].name, level, i + 1 < count ? "," : "");
    }
    std::fputs ("}\n", f); std::fclose (f);
    return 0;
}
