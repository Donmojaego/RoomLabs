// Niveau de chaque preset pour une crête vers -6 dBFS -> presets/levels.json
// (sans tic-tac ni écho, pour mesurer l'instrument ; tambour : moyenne de plusieurs touches)
#include "engine/WindupEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
struct P { const char* name; int root; float v[19]; };
static const P presets[] = {
#include "factory_presets.inc"
};
#include "params.inc"
int main (int argc, char** argv)
{
    FILE* f = std::fopen (argc > 1 ? argv[1] : "levels.json", "wb");
    std::fputs ("{\n", f);
    const int count = (int) (sizeof presets / sizeof presets[0]);
    for (int i = 0; i < count; ++i)
    {
        auto q = toParams (presets[i].v); q.levelDb = 0; q.tickOn = false;
        const bool drum = q.inst == wd::Inst::Drum;
        double pk = 1e-9;
        for (int k = 0; k < (drum ? 12 : 1); ++k)
        {
            wd::WindupEngine e; e.prepare (48000, 512); e.setParams (q); e.setTransport (120, -1);
            std::vector<float> L (48000), R (48000);
            e.noteOn (presets[i].root + k, 0.9f); e.render (L.data(), R.data(), 24000); e.noteOff (presets[i].root + k); e.render (L.data() + 24000, R.data() + 24000, 24000);
            for (size_t s = 0; s < L.size(); ++s) pk = std::max ({ pk, (double) std::abs (L[s]), (double) std::abs (R[s]) });
        }
        std::fprintf (f, "  \"%s\": %.1f%s\n", presets[i].name, std::clamp (-6.0 - 20 * std::log10 (pk), -30.0, 12.0), i + 1 < count ? "," : "");
    }
    std::fputs ("}\n", f); std::fclose (f);
}
