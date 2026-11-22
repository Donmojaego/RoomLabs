#pragma once
#include "engine/SynthEngine.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
struct PresetDef { const char* id; const char* name; void (*apply) (kal::SynthParams&); float levelDb; };
static const PresetDef kPresets[] = {
#include "presets.inc"
};
static constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

// même phrase de test pour tous : accord tenu + petite mélodie, dans le registre du preset
struct Measure { double rmsDb, peak; bool finite; };
inline Measure renderPhrase (const PresetDef& d, float levelDb, double sr = 48000)
{
    kal::SynthEngine e; e.prepare (sr, 256); kal::SynthParams p; d.apply (p); p.levelDb = levelDb; p.master = 0; e.setParams (p); e.setTempo (110);
    const std::string id = d.id; const int base = id.rfind ("bass", 0) == 0 ? 36 : (id.rfind ("lead", 0) == 0 ? 67 : 60);
    const bool mono = p.mono;
    std::vector<float> L (256), R (256); double sum = 0, pk = 0; long n = 0; bool ok = true;
    const int blocks = (int) (sr * 6 / 256);
    const int mel[8] = { 0, 4, 7, 12, 11, 7, 4, 2 };
    for (int b = 0; b < blocks; ++b)
    {
        const double t = b * 256 / sr;
        if (b == 0) { if (mono) e.noteOn (base, 0.8f); else for (int k : { 0, 4, 7 }) e.noteOn (base + k, 0.75f); }
        if (b == (int) (2.0 * sr / 256)) e.allNotesOff();
        for (int k = 0; k < 8; ++k)
        {
            const int on = (int) ((2.2 + k * 0.4) * sr / 256), off = (int) ((2.2 + k * 0.4 + 0.33) * sr / 256);
            if (b == on) e.noteOn (base + 12 * (mono ? 0 : 0) + mel[k], 0.8f);
            if (b == off) e.noteOff (base + mel[k]);
        }
        e.render (L.data(), R.data(), 256);
        if (t > 0.05 && t < 5.6)
            for (int i = 0; i < 256; ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) ok = false; sum += 0.5 * (L[i] * L[i] + R[i] * R[i]); pk = std::max (pk, (double) std::max (std::abs (L[i]), std::abs (R[i]))); ++n; }
    }
    return { 10 * std::log10 (sum / std::max (1L, n) + 1e-20), pk, ok };
}
