#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "engine/Instruments.h"

// Paramètres d'African Riddim — identifiants FIGÉS (sessions des utilisateurs).
// Le layout (Bronze / Multicolore) et la taille sont des préférences d'interface, pas des paramètres.
namespace afr::params
{
inline juce::StringArray instrumentNames()
{
    juce::StringArray s;
    for (auto& i : kInst) s.add (juce::String::fromUTF8 (i.country) + " - " + juce::String::fromUTF8 (i.name));
    return s;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    using F = juce::AudioParameterFloat; using PID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "instrument", 1 }, "Instrument", instrumentNames(), 25));
    l.add (std::make_unique<F> (PID { "volume", 1 }, "Volume", juce::NormalisableRange<float> (-36.f, 6.f, 0.f, 1.f), 0.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<F> (PID { "attack", 1 }, "Attaque", juce::NormalisableRange<float> (0.f, 2000.f, 0.f, 0.35f), 0.f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<F> (PID { "release", 1 }, "Relâchement", juce::NormalisableRange<float> (10.f, 6000.f, 0.f, 0.35f), 400.f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<juce::AudioParameterInt> (PID { "octave", 1 }, "Octave", -3, 3, 0));
    l.add (std::make_unique<F> (PID { "tune", 1 }, "Transposition", juce::NormalisableRange<float> (-12.f, 12.f, 1.f), 0.f, juce::AudioParameterFloatAttributes().withLabel ("st")));
    l.add (std::make_unique<F> (PID { "tone", 1 }, "Couleur", juce::NormalisableRange<float> (0.f, 1.f), 0.5f));
    l.add (std::make_unique<F> (PID { "space", 1 }, "Espace", juce::NormalisableRange<float> (0.f, 1.f), 0.25f));
    l.add (std::make_unique<F> (PID { "velsens", 1 }, "Toucher", juce::NormalisableRange<float> (0.f, 1.f), 0.7f));
    return l;
}
} // namespace afr::params
