#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de Smoke — identifiants FIGÉS (ne jamais les renommer : sessions des utilisateurs).
// Le thème (prune / bleu / vert) et le mode d'animation ne sont pas des paramètres : préférences d'interface.
// =============================================================================
namespace smk::params
{
struct FloatDef { const char* id; const char* name; float min, max, def, skew; const char* unit; };
struct ChoiceDef { const char* id; const char* name; std::vector<const char*> choices; int def; };

inline const std::vector<FloatDef>& floats()
{
    static const std::vector<FloatDef> f {
        { "master",  "Master",    -24.f,    6.f,    0.f,  1.f,   "dB" },   // volume général (hors presets), 0 dB par défaut
        { "tone",    "Tone",       0.f,     1.f,    0.7f, 1.f,  "" },
        { "space",   "Space",      0.f,     1.f,    0.3f, 1.f,  "" },
        { "mod",     "Mod",        0.f,     1.f,    0.3f, 1.f,  "" },
        { "dream",   "Dream",      0.f,     1.f,    0.2f, 1.f,  "" },
        { "clean",   "Clean",      0.f,     1.f,    0.5f, 1.f,  "" },
        { "level",   "Level",    -30.f,    12.f,   -6.f,  1.f,  "dB" },
        { "attack",  "Attack",     0.f,  5000.f,    5.f,  0.3f, "ms" },
        { "decay",   "Decay",      5.f,  8000.f, 1500.f,  0.3f, "ms" },
        { "sustain", "Sustain",    0.f,     1.f,    0.6f, 1.f,  "" },
        { "release", "Release",    5.f, 12000.f,  500.f,  0.3f, "ms" },
        { "bright",  "Colour",     0.f,     1.f,    0.5f, 1.f,  "" },
        { "tremolo", "Tremolo",    0.f,     1.f,    0.f,  1.f,  "" },
        { "rise",    "Rise Time",  0.5f,   16.f,    4.f,  0.5f, "s" },
        { "glide",   "Glide",      0.f,  1000.f,   80.f,  0.4f, "ms" },
        { "tune",    "Tune",     -24.f,    24.f,    0.f,  1.f,  "st" },
    };
    return f;
}

inline const std::vector<ChoiceDef>& choices()
{
    static const std::vector<ChoiceDef> c {
        { "model", "Model", { "Rhodes", "Felt Piano", "Pad", "Riser", "Sub", "House Bass" }, 0 },
        { "chord", "Chord", { "Off", "m9", "maj9", "m11", "9", "maj7" }, 0 },
    };
    return c;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (auto& d : floats())
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 }, d.name,
                   juce::NormalisableRange<float> (d.min, d.max, 0.f, d.skew), d.def, juce::AudioParameterFloatAttributes().withLabel (d.unit)));
    for (auto& d : choices())
    {
        juce::StringArray s; for (auto* x : d.choices) s.add (x);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { d.id, 1 }, d.name, s, d.def));
    }
    return l;
}
} // namespace smk::params
