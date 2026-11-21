#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de Windup Dogs — identifiants FIGÉS (ne jamais les renommer : sessions des utilisateurs).
// La tension du ressort n'est pas un paramètre : c'est l'état du jouet (sauvegardé avec le projet).
// =============================================================================
namespace wd::params
{
struct FloatDef { const char* id; const char* name; float min, max, def, skew; const char* unit; };
struct ChoiceDef { const char* id; const char* name; std::vector<const char*> choices; int def; };

inline const std::vector<FloatDef>& floats()
{
    static const std::vector<FloatDef> f {
        { "tone",       "Tone",        0.f,     1.f,   0.75f, 1.f,  "" },
        { "meca",       "Meca",        0.f,     1.f,   0.35f, 1.f,  "" },
        { "wobble",     "Wobble",      0.f,     1.f,   0.15f, 1.f,  "" },
        { "echo",       "Echo",        0.f,     1.f,   0.15f, 1.f,  "" },
        { "space",      "Space",       0.f,     1.f,   0.3f,  1.f,  "" },
        { "level",      "Level",     -30.f,    12.f,  -6.f,   1.f,  "dB" },
        { "attack",     "Attack",      0.f,  5000.f,   1.f,   0.3f, "ms" },
        { "decay",      "Decay",       5.f,  8000.f, 2500.f,  0.3f, "ms" },
        { "sustain",    "Sustain",     0.f,     1.f,   0.f,   1.f,  "" },
        { "release",    "Release",     5.f, 12000.f,  600.f,  0.3f, "ms" },
        { "bright",     "Colour",      0.f,     1.f,   0.5f,  1.f,  "" },
        { "tune",       "Tune",      -24.f,    24.f,   0.f,   1.f,  "st" },
        { "tickLevel",  "Tick Level",  0.f,     1.f,   0.4f,  1.f,  "" },
        { "swing",      "Swing",       0.f,   0.6f,   0.f,   1.f,  "" },
        { "springTime", "Spring Time", 2.f,   120.f,  40.f,   0.5f, "s" },
    };
    return f;
}

inline const std::vector<ChoiceDef>& choices()
{
    static const std::vector<ChoiceDef> c {
        { "inst",     "Instrument", { "Music Box", "Toy Piano", "Bells", "Fanfare", "Toy Drum" }, 0 },
        { "tickOn",   "Tick-Tock",  { "Off", "On" }, 0 },
        { "tickDiv",  "Tick Division", { "1/4", "1/8", "1/16", "1/8T" }, 1 },
        { "springOn", "Spring",     { "Off", "On" }, 1 },
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
} // namespace wd::params
