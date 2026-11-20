#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de Candy Shop — identifiants FIGÉS (automation, presets, interface).
// Le thème (Néon, Vert jour, Orange jour) n'est pas un paramètre : préférence d'interface.
// =============================================================================
namespace cdy::params
{
struct FloatDef { const char* id; const char* name; float min, max, def, skew; const char* unit; };
struct ChoiceDef { const char* id; const char* name; std::vector<const char*> choices; int def; };

inline const std::vector<FloatDef>& floats()
{
    static const std::vector<FloatDef> f {
        // façade : A D S R, Tone, Filter, Space, Level
        { "attack",    "Attack",      0.f,  5000.f,    2.f, 0.3f,  "ms" },
        { "decay",     "Decay",       5.f,  5000.f,  300.f, 0.3f,  "ms" },
        { "sustain",   "Sustain",     0.f,     1.f,   0.7f, 1.f,   "" },
        { "release",   "Release",     5.f,  5000.f,  250.f, 0.3f,  "ms" },
        { "tone",      "Tone",       -1.f,     1.f,    0.f, 1.f,   "" },
        { "cutoff",    "Filter",     60.f, 18000.f, 8000.f, 0.25f, "Hz" },
        { "space",     "Space",       0.f,     1.f,   0.2f, 1.f,   "" },
        { "level",     "Level",     -36.f,    12.f,   -6.f, 1.f,   "dB" },
        // avancé
        { "detune",    "Detune",      0.f,    50.f,    8.f, 1.f,   "ct" },
        { "fmRatio",   "FM Ratio",    0.5f,   12.f,   3.5f, 0.5f,  "" },
        { "fmAmount",  "FM Amount",   0.f,     1.f,    0.f, 1.f,   "" },
        { "fmDecay",   "FM / Mallet Decay", 0.02f, 3.f, 0.4f, 0.4f, "s" },
        { "noise",     "Noise",       0.f,     1.f,    0.f, 1.f,   "" },
        { "pitchDrop", "Pitch Drop",  0.f,    24.f,    0.f, 1.f,   "st" },
        { "tune",      "Tune",      -24.f,    24.f,    0.f, 1.f,   "st" },
        { "reso",      "Resonance",   0.f,     1.f,   0.1f, 1.f,   "" },
        { "fenv",      "Filter Env",  0.f,     1.f,   0.3f, 1.f,   "" },
        { "fdecay",    "Filter Decay",5.f,  3000.f,  250.f, 0.3f,  "ms" },
        { "chorus",    "Chorus",      0.f,     1.f,    0.f, 1.f,   "" },
        { "velSens",   "Velocity",    0.f,     1.f,   0.6f, 1.f,   "" },
        { "arpGate",   "Arp Gate",    0.1f,    1.f,   0.6f, 1.f,   "" },
        { "drumLevel", "Drum Level", -36.f,    12.f,  -4.f, 1.f,   "dB" },
    };
    return f;
}

inline const std::vector<ChoiceDef>& choices()
{
    static const std::vector<ChoiceDef> c {
        { "algo",    "Engine",   { "Mallet", "Synth", "FM" }, 1 },
        { "wave",    "Wave",     { "Sine", "Triangle", "Saw", "Square" }, 2 },
        { "unison",  "Unison",   { "1", "2", "3" }, 1 },
        { "chord",   "Chord",    { "Off", "Major", "Minor", "Maj7", "m7", "Sus4", "Power", "Add9" }, 0 },
        { "arpOn",   "Arp",      { "Off", "On" }, 0 },
        { "arpRate", "Arp Rate", { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3 },
        { "arpMode", "Arp Mode", { "Up", "Down", "Up/Down", "Random", "As Played" }, 0 },
        { "arpOct",  "Arp Octaves", { "1", "2", "3" }, 0 },
        { "kit",     "Drum Kit", { "Bocal", "Sucre Sec", "Caramel Mou", "Petillant", "Mecanique" }, 0 },
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
        juce::StringArray s; for (auto* x : d.choices) s.add (x);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { d.id, 1 }, d.name, s, d.def));
    }
    return l;
}
} // namespace cdy::params
