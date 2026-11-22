#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
// Paramètres de Kaleidoscope — identifiants FIGÉS (sessions des utilisateurs).
// Les paramètres de son portent le même nom que les champs de kal::SynthParams (presets = tools/presets.py).
// =============================================================================
namespace kal::params
{
enum class Kind { Float, Int, Bool, Choice };
struct Def { const char* id; const char* name; Kind kind; float min, max, def, skew; const char* unit; std::vector<const char*> choices {}; };

inline const std::vector<const char*>& tableNames() { static const std::vector<const char*> t { "Analogique", "Impulsion", "Formants", "Verre", "Orgue", "Numérique", "Chœur", "Spectral" }; return t; }

inline const std::vector<Def>& sound()
{
    using K = Kind;
    static const std::vector<Def> d {
        { "wtOn", "Oscillateurs", K::Bool, 0, 1, 1, 1, "" },
        { "tableA", "Table A", K::Choice, 0, 7, 0, 1, "", tableNames() },
        { "posA", "Position A", K::Float, 0, 1, 0.3f, 1, "" },
        { "tableB", "Table B", K::Choice, 0, 7, 1, 1, "", tableNames() },
        { "posB", "Position B", K::Float, 0, 1, 0.5f, 1, "" },
        { "mixB", "Mélange B", K::Float, 0, 1, 0.4f, 1, "" },
        { "semiB", "Demi-tons B", K::Float, -24, 24, 0, 1, "st" },
        { "fineB", "Finesse B", K::Float, -50, 50, 7, 1, "ct" },
        { "unison", "Unisson", K::Int, 1, 4, 2, 1, "" },
        { "detune", "Désaccord", K::Float, 0, 1, 0.2f, 1, "" },
        { "spread", "Largeur unisson", K::Float, 0, 1, 0.6f, 1, "" },
        { "sub", "Sub", K::Float, 0, 1, 0.2f, 1, "" },
        { "noise", "Bruit", K::Float, 0, 1, 0, 1, "" },
        { "wtEnv", "Enveloppe > table", K::Float, -1, 1, 0, 1, "" },
        { "wtLfo", "LFO > table", K::Float, 0, 1, 0, 1, "" },
        { "fMode", "Filtre", K::Choice, 0, 2, 0, 1, "", { "Passe-bas", "Passe-bande", "Passe-haut" } },
        { "f24", "Pente 24 dB", K::Bool, 0, 1, 0, 1, "" },
        { "cutoff", "Coupure", K::Float, 0, 1, 0.7f, 1, "" },
        { "reso", "Résonance", K::Float, 0, 1, 0.2f, 1, "" },
        { "fEnv", "Enveloppe filtre", K::Float, -1, 1, 0.3f, 1, "" },
        { "keytrack", "Suivi clavier", K::Float, 0, 1, 0.5f, 1, "" },
        { "drive", "Chaleur", K::Float, 0, 1, 0.1f, 1, "" },
        { "aA", "Attaque", K::Float, 0.5f, 8000, 5, 0.3f, "ms" },
        { "aD", "Déclin", K::Float, 5, 10000, 600, 0.3f, "ms" },
        { "aS", "Maintien", K::Float, 0, 1, 0.7f, 1, "" },
        { "aR", "Relâche", K::Float, 5, 12000, 400, 0.3f, "ms" },
        { "fA", "Attaque filtre", K::Float, 0.5f, 8000, 2, 0.3f, "ms" },
        { "fD", "Déclin filtre", K::Float, 5, 10000, 500, 0.3f, "ms" },
        { "fS", "Maintien filtre", K::Float, 0, 1, 0.3f, 1, "" },
        { "fR", "Relâche filtre", K::Float, 5, 12000, 400, 0.3f, "ms" },
        { "lfoRate", "Vitesse LFO", K::Float, 0.02f, 20, 4, 0.4f, "Hz" },
        { "lfoVib", "Vibrato", K::Float, 0, 1, 0, 1, "" },
        { "lfoCut", "LFO > filtre", K::Float, 0, 1, 0, 1, "" },
        { "lfoAmp", "Trémolo", K::Float, 0, 1, 0, 1, "" },
        { "granOn", "Granulaire", K::Bool, 0, 1, 0, 1, "" },
        { "granSize", "Taille grain", K::Float, 10, 400, 80, 0.5f, "ms" },
        { "granDensity", "Densité grains", K::Float, 2, 80, 30, 0.6f, "/s" },
        { "granSpray", "Dispersion", K::Float, 0, 1, 0.3f, 1, "" },
        { "granPitch", "Hauteur aléatoire", K::Float, 0, 1, 0, 1, "" },
        { "granPos", "Position grains", K::Float, 0, 1, 0.3f, 1, "" },
        { "granMix", "Mélange grains", K::Float, 0, 1, 0.5f, 1, "" },
        { "smpOn", "Fond", K::Bool, 0, 1, 0, 1, "" },
        { "smpLevel", "Niveau fond", K::Float, 0, 1.5f, 0.8f, 1, "" },
        { "smpTune", "Accord fond", K::Float, -24, 24, 0, 1, "st" },
        { "mono", "Mono", K::Bool, 0, 1, 0, 1, "" },
        { "glideMs", "Glissé", K::Float, 0, 1000, 0, 0.4f, "ms" },
        { "bendRange", "Pitch bend", K::Float, 0, 24, 2, 1, "st" },
        { "fxDrive", "Saturation", K::Float, 0, 1, 0, 1, "" },
        { "chorus", "Chorus", K::Float, 0, 1, 0, 1, "" },
        { "delay", "Écho", K::Float, 0, 1, 0, 1, "" },
        { "delayFb", "Répétitions", K::Float, 0, 0.92f, 0.35f, 1, "" },
        { "delayDiv", "Division écho", K::Choice, 0, 5, 2, 1, "", { "1/16", "1/8", "1/8 pointée", "1/4", "1/4 pointée", "1/2" } },
        { "reverb", "Espace", K::Float, 0, 1, 0.2f, 1, "" },
        { "revSize", "Taille espace", K::Float, 0, 1, 0.6f, 1, "" },
        { "width", "Largeur", K::Float, 0, 1.5f, 0.8f, 1, "" },
        { "tilt", "Couleur", K::Float, -1, 1, 0, 1, "" },
        { "levelDb", "Volume", K::Float, -30, 24, -6, 1, "dB" },
    };
    return d;
}

#include "gen/Names.inc"

inline const std::vector<Def>& generator()
{
    using K = Kind;
    static const std::vector<Def> d {
        { "style", "Style", K::Choice, 0, 0, 0, 1, "", styleNames() },
        { "mood", "Humeur", K::Choice, 0, 0, 0, 1, "", moodNames() },
        { "genMode", "Rôle", K::Choice, 0, 1, 0, 1, "", { "Solo", "Accompagnement" } },
        { "keyMode", "Tonalité", K::Choice, 0, 1, 0, 1, "", { "Keys", "Drop" } },
        { "root", "Tonique", K::Choice, 0, 11, 0, 1, "", { "Do", "Do#", "Ré", "Mib", "Mi", "Fa", "Fa#", "Sol", "Lab", "La", "Sib", "Si" } },
        { "scale", "Gamme", K::Choice, 0, 20, 0, 1, "", { "Auto", "Majeur", "Mineur naturel", "Mineur harmonique", "Mineur mélodique", "Dorien", "Phrygien",
            "Lydien", "Mixolydien", "Locrien", "Penta mineure", "Penta majeure", "Blues", "Phrygien dominant", "Double harmonique", "Hirajoshi",
            "In sen", "Bhairav", "Kafi", "Par tons", "Hongroise mineure" } },
        { "bars", "Mesures", K::Choice, 0, 3, 2, 1, "", { "1", "2", "4", "8" } },
        { "density", "Densité", K::Float, -1, 1, 0, 1, "" },
        { "octave", "Octave", K::Int, -2, 2, 0, 1, "" },
        { "variation", "Déclinaison", K::Float, 0, 1, 0.5f, 1, "" },
        { "personal", "Personal", K::Float, 0, 1, 0.5f, 1, "" },
        { "humanize", "Humain", K::Float, 0, 1, 0.3f, 1, "" },
        { "gate", "Longueur notes", K::Float, 0.2f, 1.5f, 1, 1, "" },
        { "play", "Lecture", K::Bool, 0, 1, 0, 1, "" },
        { "follow", "Suivre l'hôte", K::Bool, 0, 1, 1, 1, "" },
        { "tempo", "Tempo libre", K::Float, 50, 200, 110, 1, "BPM" },
        { "evolve", "Infini", K::Choice, 0, 4, 0, 1, "", { "Arrêt", "Chaque boucle", "Toutes les 2", "Toutes les 4", "Toutes les 8" } },
        { "midiOut", "Sortie MIDI", K::Bool, 0, 1, 1, 1, "" },
        { "master", "Master", K::Float, -36, 12, -4, 1, "dB" },
    };
    return d;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto add = [&] (const Def& d)
    {
        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case Kind::Float: l.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, juce::NormalisableRange<float> (d.min, d.max, 0.f, d.skew), d.def,
                                         juce::AudioParameterFloatAttributes().withLabel (d.unit))); break;
            case Kind::Int: l.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def)); break;
            case Kind::Bool: l.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f)); break;
            case Kind::Choice: { juce::StringArray s; for (auto* c : d.choices) s.add (juce::String::fromUTF8 (c));
                                 l.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, s, (int) d.def)); break; }
        }
    };
    for (auto& d : sound()) add (d);
    for (auto& d : generator()) add (d);
    return l;
}
} // namespace kal::params
