// =============================================================================
// Smoke — test du plugin complet hors DAW (avec JUCE, sans interface).
//   SmokeRenderTest [RAPPORT.md]
// Charge chaque preset par le processeur, joue sa note de référence en MIDI,
// vérifie le son, la sauvegarde/le rappel de l'état et les tailles de bloc.
// =============================================================================
#include "PluginProcessor.h"
#include <cmath>

static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::String rep = U ("\n## Plugin complet (processeur JUCE)\n\n");
    int failures = 0;
    auto line = [&] (bool ok, const juce::String& s) { if (! ok) ++failures; rep << (ok ? U ("- [OK] ") : U ("- [ÉCHEC] ")) << s << "\n"; };

    SmokeProcessor proc;
    const auto& presets = proc.factory();
    line (presets.size() >= 40, juce::String ((int) presets.size()) + U (" presets d'usine chargés"));

    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 32, 256, 1024 })
        {
            proc.setRateAndBufferSizeDetails (sr, block);
            proc.prepareToPlay (sr, block);
            int bad = 0;
            for (int i = 0; i < (int) presets.size(); ++i)
            {
                proc.loadPreset (i);
                juce::AudioBuffer<float> buf (2, block);
                juce::MidiBuffer midi;
                double peak = 0, energy = 0; bool finite = true;
                const int total = (int) (sr * 0.6);
                for (int pos = 0; pos < total; pos += block)
                {
                    midi.clear();
                    if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, presets[(size_t) i].root, (juce::uint8) 110), 3);
                    if (pos <= (int) (sr * 0.4) && pos + block > (int) (sr * 0.4))
                        midi.addEvent (juce::MidiMessage::noteOff (1, presets[(size_t) i].root), (int) (sr * 0.4) - pos);
                    proc.processBlock (buf, midi);
                    for (int c = 0; c < 2; ++c)
                        for (int s = 0; s < block; ++s)
                        {
                            const float v = buf.getSample (c, s);
                            finite &= std::isfinite (v);
                            peak = juce::jmax (peak, (double) std::abs (v));
                            energy += v * v;
                        }
                }
                if (! finite || peak > 1.0 || energy < 1e-3) ++bad;
                // repos entre deux presets
                juce::MidiBuffer off; off.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                for (int k = 0; k < (int) (sr * 1.5) / block; ++k) { proc.processBlock (buf, off); off.clear(); }
            }
            line (bad == 0, juce::String (sr / 1000.0, 1) + U (" kHz, blocs de ") + juce::String (block)
                               + U (" : tous les presets sonnent, sans NaN ni crête au-dessus de 0 dBFS")
                               + (bad ? " (" + juce::String (bad) + U (" en échec)") : juce::String()));
        }

    // accord m9 et dissipation de la fumée à travers le processeur
    {
        proc.setRateAndBufferSizeDetails (48000, 256); proc.prepareToPlay (48000, 256);
        proc.loadPreset (2);   // Accord m9 Feutré
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi; midi.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0);
        proc.processBlock (buf, midi);
        const int voices = proc.feedback().voices.load(), held = proc.feedback().held.load();
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 57), 0); proc.processBlock (buf, off);
        for (int k = 0; k < 48000 * 15 / 256; ++k) { juce::MidiBuffer none; proc.processBlock (buf, none); }
        line (voices == 5 && held == 1 && proc.feedback().held.load() == 0 && proc.feedback().energy.load() < 0.01f,
              U ("processeur : une touche joue l'accord m9 (") + juce::String (voices) + U (" voix), la fumée retombe après la relâche"));
    }

    // sauvegarde / rappel de l'état
    {
        proc.loadPreset (12);
        if (auto* prm = proc.state.getParameter ("space")) prm->setValueNotifyingHost (0.77f);
        proc.setProjectUi ("theme", "vert");
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        SmokeProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool ok = other.currentPreset.load() == 12
                     && std::abs (other.state.getParameter ("space")->getValue() - 0.77f) < 1e-4f
                     && other.getProjectUi().getProperty ("theme", "").toString() == "vert";
        line (ok, U ("état du projet sauvegardé et rappelé (preset, réglages, thème)"));
    }

    std::fputs (rep.toRawUTF8(), stdout);
    if (argc > 1)
    {
        juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
        f.appendText (rep, false, false, "\n");
    }
    return failures ? 1 : 0;
}
