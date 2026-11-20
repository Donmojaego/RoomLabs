#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de Shake It.
// Les identifiants sont FIGÉS : automation, presets (presets/factory.json) et
// interface (ui/index.html). Ne jamais renommer un identifiant existant.
// La tenue, le thème et le mode visuel ne sont PAS des paramètres : ce sont des
// préférences d'interface, sans effet sur le son.
// =============================================================================
namespace shk::params
{
struct FloatDef { const char* id; const char* name; float min, max, def, skew; const char* unit; };
struct ChoiceDef { const char* id; const char* name; std::vector<const char*> choices; int def; };

inline const std::vector<FloatDef>& floats()
{
    static const std::vector<FloatDef> f {
        { "tune",    "Tune",        -24.f,   24.f,     0.f,  1.f,   "st" },
        { "glide",   "Glide",         0.f, 1000.f,   120.f,  0.45f, "ms" },
        { "punch",   "Punch",         0.f,    1.f,    0.5f,  1.f,   "" },
        { "decay",   "Decay",       0.05f,    8.f,    1.5f,  0.4f,  "s" },
        { "drive",   "Drive",         0.f,    1.f,     0.f,  1.f,   "" },
        { "output",  "Output",      -36.f,    6.f,    -3.f,  1.f,   "dB" },
        { "harm",    "Harmonics",     0.f,    1.f,     0.f,  1.f,   "" },
        { "tone",    "Tone",         40.f, 18000.f, 18000.f, 0.25f, "Hz" },
        { "reso",    "Resonance",     0.f,    1.f,     0.f,  1.f,   "" },
        { "attack",  "Attack",        0.f,  200.f,     1.f,  0.4f,  "ms" },
        { "release", "Release",       5.f, 2000.f,   120.f,  0.4f,  "ms" },
    };
    return f;
}

inline const std::vector<ChoiceDef>& choices()
{
    static const std::vector<ChoiceDef> c {
        { "model",     "Model",      { "808", "Sub", "Moog-style", "Pluck" }, 0 },
        { "glideMode", "Glide Mode", { "Always", "Legato" }, 1 },
        { "driveType", "Drive Type", { "Soft", "Hard", "Fold" }, 0 },
    };
    return c;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (auto& d : floats())
    {
        juce::NormalisableRange<float> r (d.min, d.max, 0.f, d.skew);
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 }, d.name, r, d.def,
                   juce::AudioParameterFloatAttributes().withLabel (d.unit)));
    }
    for (auto& d : choices())
    {
        juce::StringArray s;
        for (auto* x : d.choices) s.add (x);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { d.id, 1 }, d.name, s, d.def));
    }
    return l;
}
} // namespace shk::params
