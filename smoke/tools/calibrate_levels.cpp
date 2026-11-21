// Niveau de chaque preset pour une crête vers -6 dBFS -> presets/levels.json
#include "engine/SmokeEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
struct P { const char* name; int root; float v[17]; };
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
        auto q = toParams (presets[i].v); q.levelDb = 0;
        smk::SmokeEngine e; e.prepare (48000, 512); e.setParams (q);
        std::vector<float> L (48000 * 2), R (48000 * 2);
        e.noteOn (presets[i].root, 0.9f); e.render (L.data(), R.data(), 48000); e.noteOff (presets[i].root); e.render (L.data() + 48000, R.data() + 48000, 48000);
        double pk = 1e-9; for (size_t k = 0; k < L.size(); ++k) pk = std::max ({ pk, (double) std::abs (L[k]), (double) std::abs (R[k]) });
        std::fprintf (f, "  \"%s\": %.1f%s\n", presets[i].name, std::clamp (-6.0 - 20 * std::log10 (pk), -30.0, 12.0), i + 1 < count ? "," : "");
    }
    std::fputs ("}\n", f); std::fclose (f);
}
