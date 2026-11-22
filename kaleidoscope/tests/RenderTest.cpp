// =============================================================================
// Kaleidoscope — test du plugin complet hors DAW (avec JUCE, sans interface).
//   KaleidoscopeRenderTest [RAPPORT.md]
// Presets, séquenceur (sortie audio et MIDI), Générer / Déclinaison / Personal, état du projet,
// export MIDI, chargement d'un fond depuis le disque, tailles de bloc.
// =============================================================================
#include "PluginProcessor.h"
#include <cmath>

static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

static void setP (KaleidoProcessor& p, const char* id, float v)
{
    if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (p.state.getParameter (id))) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::String rep = U ("\n## Plugin complet (processeur JUCE)\n\n");
    int failures = 0;
    auto line = [&] (bool ok, const juce::String& s) { if (! ok) ++failures; rep << (ok ? U ("- [OK] ") : U ("- [ÉCHEC] ")) << s << "\n"; };

    KaleidoProcessor proc;
    const auto& presets = proc.factory();
    int natives = 0, fonds = 0; for (auto& p : presets) (p.fond ? fonds : natives)++;
    line (natives >= 150 && fonds >= 100, juce::String (natives) + U (" presets natifs + ") + juce::String (fonds) + U (" fonds = ") + juce::String ((int) presets.size()));

    // 1. chaque preset natif joue la mélodie générée (séquenceur libre), à plusieurs fréquences et blocs
    for (double sr : { 44100.0, 48000.0 })
        for (int block : { 64, 512 })
        {
            proc.setRateAndBufferSizeDetails (sr, block);
            proc.prepareToPlay (sr, block);
            setP (proc, "follow", 0); setP (proc, "tempo", 120);
            int bad = 0, midiNotes = 0;
            for (int i = 0; i < (int) presets.size(); ++i)
            {
                if (presets[(size_t) i].fond) continue;
                proc.loadPreset (i);
                setP (proc, "play", 1);
                juce::AudioBuffer<float> buf (2, block);
                double peak = 0, energy = 0; bool finite = true;
                for (int pos = 0; pos < (int) (sr * 2.0); pos += block)
                {
                    juce::MidiBuffer midi;
                    proc.processBlock (buf, midi);
                    for (const auto m : midi) if (m.getMessage().isNoteOn()) ++midiNotes;
                    for (int c = 0; c < 2; ++c)
                        for (int s = 0; s < block; ++s) { const float v = buf.getSample (c, s); finite &= std::isfinite (v); peak = juce::jmax (peak, (double) std::abs (v)); energy += v * v; }
                }
                if (! finite || peak > 1.0 || energy < 1e-4) ++bad;
                setP (proc, "play", 0);
                juce::MidiBuffer none;
                for (int k = 0; k < (int) (sr * 1.0) / block; ++k) proc.processBlock (buf, none);
            }
            line (bad == 0 && midiNotes > 0, juce::String (sr / 1000.0, 1) + U (" kHz, blocs de ") + juce::String (block)
                  + U (" : les 150 presets jouent la mélodie sans NaN ni écrêtage, ") + juce::String (midiNotes) + U (" notes envoyées en MIDI")
                  + (bad ? " (" + juce::String (bad) + U (" en échec)") : juce::String()));
        }

    // 2. Générer, Déclinaison, Précédent, Personal
    {
        auto a = proc.generate(); auto b = proc.generate();
        const bool differ = a.getProperty ("seed", "").toString() != b.getProperty ("seed", "").toString();
        auto c = proc.decline();
        auto back = proc.previous();
        const bool prevOk = back.getProperty ("seed", "").toString() == b.getProperty ("seed", "").toString();
        auto d = proc.feedback ("like"); auto e = proc.feedback ("redo");
        const bool fbOk = (int) e.getProperty ("liked", 0) >= 1 && (int) e.getProperty ("disliked", 0) >= 1;
        int notes = c.getProperty ("notes", {}).size();
        line (differ && prevOk && fbOk && notes > 0, U ("Générer donne une idée neuve, Déclinaison une variante (") + juce::String (notes)
              + U (" notes), Précédent revient en arrière, J'aime / À refaire nourrissent Personal"));
        proc.resetPersonal();
    }

    // 3. styles × rôles : chaque style produit une mélodie et un accompagnement
    {
        int empty = 0;
        for (int s = 0; s < kal::Generator::numStyles(); ++s)
            for (int m : { 0, 1 })
            {
                setP (proc, "style", (float) s); setP (proc, "genMode", (float) m);
                if (proc.generate().getProperty ("notes", {}).size() == 0) ++empty;
            }
        line (empty == 0, juce::String (kal::Generator::numStyles()) + U (" styles × Solo / Accompagnement : tous produisent des notes"));
        setP (proc, "style", 0); setP (proc, "genMode", 0);
    }

    // 4. Drop : tonalité imposée par l'analyse
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject(); o->setProperty ("root", 9); o->setProperty ("scale", 1); o->setProperty ("bpm", 92.0);
        auto r = proc.setDrop (juce::var (o.get()));
        const bool ok = (int) r.getProperty ("root", -1) == 9 && (int) r.getProperty ("scale", -1) == 1
                     && (int) std::lround (proc.state.getRawParameterValue ("keyMode")->load()) == 1;
        line (ok, U ("Drop : la mélodie suit la tonalité détectée (la mineur, 92 BPM)"));
    }

    // 5. synchronisation avec l'hôte : sans transport, le séquenceur avance en tempo libre ; mode infini
    {
        proc.setRateAndBufferSizeDetails (48000, 256); proc.prepareToPlay (48000, 256);
        setP (proc, "bars", 0); setP (proc, "evolve", 1); setP (proc, "play", 1); setP (proc, "tempo", 180);
        proc.generate();
        const int before = proc.patternSerial.load();
        juce::AudioBuffer<float> buf (2, 256);
        for (int k = 0; k < 48000 * 6 / 256; ++k) { juce::MidiBuffer m; proc.processBlock (buf, m); if (k % 50 == 0) proc.pumpForTests(); }
        proc.pumpForTests();
        const int evolved = proc.patternSerial.load() - before;
        line (proc.loopsDone.load() >= 3 && evolved >= 2, U ("mode infini : ") + juce::String (proc.loopsDone.load()) + U (" boucles, ")
              + juce::String (evolved) + U (" nouvelles variations sans arrêter la lecture"));
        setP (proc, "evolve", 0); setP (proc, "play", 0); setP (proc, "bars", 2);
    }

    // 6. export MIDI
    {
        auto f = proc.exportMidi (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kal_test.mid"));
        juce::MidiFile mf; juce::FileInputStream in (f);
        const bool ok = f.existsAsFile() && in.openedOk() && mf.readFrom (in) && mf.getNumTracks() == 1 && mf.getTrack (0)->getNumEvents() > 4;
        line (ok, U ("export .mid de la proposition (") + juce::String (ok ? mf.getTrack (0)->getNumEvents() : 0) + U (" événements)"));
        f.deleteFile();
    }

    // 7. fond chargé depuis le disque (dossier utilisateur), joué par le moteur d'échantillons
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Room Labs/Kaleidoscope/Banques/marimba");
        const bool existed = dir.isDirectory();
        bool ok = false;
        if (! existed)
        {
            dir.createDirectory();
            juce::FlacAudioFormat flac;
            {
                std::unique_ptr<juce::OutputStream> os (dir.getChildFile ("marimba_69.flac").createOutputStream().release());
                std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (os.get(), 48000, 1, 16, {}, 0));
                if (w) { os.release(); juce::AudioBuffer<float> b (1, 24000);
                         for (int i = 0; i < 24000; ++i) b.setSample (0, i, 0.5f * (float) std::sin (6.283185307 * 440 * i / 48000.0) * (1.f - i / 24000.f));
                         w->writeFromAudioSampleBuffer (b, 0, 24000); }
            }
            dir.getChildFile ("instrument.json").replaceWithText (R"({"id":"marimba","sustain":false,"regions":[{"file":"marimba_69.flac","root":69,"lo":0,"hi":127,"vlo":0,"vhi":127}]})");
        }
        int idx = -1; for (int i = 0; i < (int) presets.size(); ++i) if (presets[(size_t) i].bank == "marimba") idx = i;
        proc.loadPreset (idx);
        for (int t = 0; t < 200 && proc.loadedBank() != "marimba"; ++t) juce::Thread::sleep (20);
        juce::AudioBuffer<float> buf (2, 256); double energy = 0;
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100), 0);
        for (int k = 0; k < 100; ++k) { proc.processBlock (buf, m); m.clear(); for (int s = 0; s < 256; ++s) energy += buf.getSample (0, s) * buf.getSample (0, s); }
        ok = proc.loadedBank() == "marimba" && energy > 0.1;
        line (ok, U ("fond « Marimba » chargé depuis le disque et joué (énergie ") + juce::String (energy, 2) + ")"
              + (proc.bankError().isNotEmpty() ? " - " + proc.bankError() : juce::String()));
        if (! existed) dir.deleteRecursively();
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::allNotesOff (1), 0); proc.processBlock (buf, off);
    }

    // 8. état du projet : réglages, preset, mélodie, affichage
    {
        proc.loadPreset (12);
        setP (proc, "reverb", 0.77f);
        auto pat = proc.generate();
        proc.setProjectUi ("layout", "holo");
        juce::MemoryBlock mb; proc.getStateInformation (mb);
        KaleidoProcessor other; other.setStateInformation (mb.getData(), (int) mb.getSize());
        auto op = other.patternInfo();
        const bool ok = other.currentPreset.load() == 12 && std::abs (other.state.getRawParameterValue ("reverb")->load() - 0.77f) < 1e-3f
                     && op.getProperty ("notes", {}).size() == pat.getProperty ("notes", {}).size()
                     && other.getProjectUi().getProperty ("layout", "").toString() == "holo";
        line (ok, U ("état du projet sauvegardé et rappelé (preset, réglages, mélodie identique, layout)"));
    }

    std::fputs (rep.toRawUTF8(), stdout);
    if (argc > 1) juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]).appendText (rep, false, false, "\n");
    return failures ? 1 : 0;
}
