#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de « Ma petite soupe ouselem » — identifiants FIGÉS.
// Le mode d'affichage (soupe / Mon petit ramen) n'est pas un paramètre : préférence d'interface.
// =============================================================================
namespace soup::params
{
struct FloatDef { const char* id; const char* name; float min, max, def, skew; const char* unit; };
struct ChoiceDef { const char* id; const char* name; std::vector<const char*> choices; int def; };

inline const std::vector<FloatDef>& floats()
{
    static const std::vector<FloatDef> f {
        { "tone",       "Tone",       -1.f,    1.f,    0.f, 1.f,   "" },
        { "damp",       "Damp",        0.f,    1.f,   0.4f, 1.f,   "" },
        { "space",      "Space",       0.f,    1.f,   0.3f, 1.f,   "" },
        { "mix",        "Mix",         0.f,    1.f,   0.5f, 1.f,   "" },
        { "feedback",   "Feedback",    0.f,  0.92f,   0.4f, 1.f,   "" },
        { "stretch",    "Stretch",     0.f,    1.f,   0.5f, 1.f,   "" },
        { "level",      "Level",     -36.f,   12.f,  -6.f,  1.f,   "dB" },
        { "attack",     "Attack",      0.f, 5000.f,  20.f,  0.3f,  "ms" },
        { "decay",      "Decay",       5.f, 5000.f, 800.f,  0.3f,  "ms" },
        { "sustain",    "Sustain",     0.f,    1.f,   0.8f, 1.f,   "" },
        { "release",    "Release",     5.f, 8000.f, 600.f,  0.3f,  "ms" },
        { "brightness", "Colour",      0.f,    1.f,   0.5f, 1.f,   "" },
        { "tune",       "Tune",      -24.f,   24.f,   0.f,  1.f,   "st" },
    };
    return f;
}

inline const std::vector<ChoiceDef>& choices()
{
    static const std::vector<ChoiceDef> c {
        { "inst",      "Instrument", { "Violin", "Pad", "Keys", "Bell", "Strings", "Texture", "Bass", "Drum" }, 0 },
        { "fx",        "FX",         { "None", "Reverse", "Stutter", "Repeat", "Slicer", "Stretch", "Filter Mod", "Grain Delay" }, 7 },
        { "division",  "Division",   { "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" }, 2 },
        { "reverseOn", "Reverse",    { "Off", "On" }, 0 },
        { "stutterOn", "Stutter",    { "Off", "On" }, 0 },
        { "arpOn",     "Arp",        { "Off", "On" }, 0 },
        { "arpMode",   "Arp Mode",   { "Up", "Down" }, 0 },
        { "arpOct",    "Arp Octaves",{ "1", "2", "3" }, 0 },
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
} // namespace soup::params
