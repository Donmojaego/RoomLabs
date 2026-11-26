// Test du plugin complet hors DAW : 54 programmes, état sauvegardé / rechargé, fréquences et tailles de bloc.
#include "PluginProcessor.h"
#include <cstdio>
static std::string rep = "\n## African Riddim — plugin (JUCE)\n\n"; static int fails = 0;
static void line (bool ok, const juce::String& s) { std::printf ("%s  %s\n", ok ? "OK " : "ÉCHEC", s.toRawUTF8()); rep += (ok ? "- [OK] " : "- [ÉCHEC] ") + s.toStdString() + "\n"; if (! ok) ++fails; }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    // 1. chaque programme (instrument) sonne, sans NaN, sous 0 dBFS
    {
        AfricanProcessor p; p.setPlayConfigDetails (0, 2, 48000, 512); p.prepareToPlay (48000, 512);
        int ok = 0; float worst = 0;
        for (int prog = 0; prog < p.getNumPrograms(); ++prog)
        {
            p.setCurrentProgram (prog);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; float pk = 0; bool fin = true;
            for (int blk = 0; blk < 120; ++blk)
            {
                m.clear(); if (blk == 1) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10); if (blk == 40) m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                p.processBlock (b, m);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = b.getSample (c, i); fin &= std::isfinite (x); pk = std::max (pk, std::abs (x)); }
            }
            ok += (fin && pk > 0.01f && pk < 1.f); worst = std::max (worst, pk);
        }
        line (ok == 54, juce::String (ok) + "/54 instruments sonnent (crête max " + juce::String (worst, 2) + ", volume par défaut 0 dB)");
    }
    // 2. état : sauvegarde puis rechargement dans une nouvelle instance
    {
        AfricanProcessor a; a.setCurrentProgram (9);
        if (auto* v = a.state.getParameter ("volume")) v->setValueNotifyingHost (v->convertTo0to1 (-4.5f));
        if (auto* o = a.state.getParameter ("octave")) o->setValueNotifyingHost (o->convertTo0to1 (1.f));
        a.setProjectUi ("country", 9);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        AfricanProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        const float vol = b.state.getRawParameterValue ("volume")->load(), oct = b.state.getRawParameterValue ("octave")->load();
        line (b.getCurrentProgram() == 9 && std::abs (vol + 4.5f) < 0.05f && oct == 1.f, "état sauvegardé et rechargé (instrument Kora, volume -4,5 dB, octave +1)");
    }
    // 3. fréquences d'échantillonnage et tailles de bloc
    {
        bool ok = true;
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (int bs : { 64, 256, 1024 })
            {
                AfricanProcessor p; p.setPlayConfigDetails (0, 2, sr, bs); p.prepareToPlay (sr, bs); p.setCurrentProgram (20);
                juce::AudioBuffer<float> b (2, bs); juce::MidiBuffer m;
                for (int blk = 0; blk < (int) (sr / bs); ++blk) { m.clear(); if (blk % 8 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 48 + blk % 24, (juce::uint8) 90), 0); p.processBlock (b, m);
                    for (int i = 0; i < bs; ++i) ok &= std::isfinite (b.getSample (0, i)) && std::abs (b.getSample (0, i)) < 1.f; }
            }
        line (ok, "44,1 / 48 / 96 kHz × blocs 64 / 256 / 1024 : son propre, sous 0 dBFS");
    }
    std::printf ("%s\n", fails ? "ÉCHECS" : "Tous les tests du plugin passent");
    if (argc > 1) { if (auto* f = std::fopen (argv[1], "a")) { std::fputs (rep.c_str(), f); std::fclose (f); } }
    return fails ? 1 : 0;
}
