// Tests du moteur sonore Kaleidoscope : 150 presets calibrés, stabilité, échantillons, granulaire, polyphonie, coût CPU.
#include "EngineCommon.h"
#include <chrono>
#include <cstdio>
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; std::printf ("ÉCHEC : " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main()
{
    std::printf ("%d presets natifs\n", kNumPresets);
    CHECK (kNumPresets >= 150, "moins de 150 presets");
    // 1. chaque preset : fini, niveau calibré, crête sous -1 dBFS
    double worst = 0;
    for (int i = 0; i < kNumPresets; ++i)
    {
        kal::SynthParams sp; kPresets[i].apply (sp);
        auto m = renderPhrase (kPresets[i], kPresets[i].levelDb);
        const double target = sp.aS < 0.1f ? -22.0 : -18.0;
        CHECK (m.finite, "%s non fini", kPresets[i].id);
        CHECK (m.peak < 0.9, "%s crête %.2f", kPresets[i].id, m.peak);
        const double err = std::abs (m.rmsDb - target);
        if (m.peak < 0.62) { CHECK (err < 1.5, "%s niveau %.1f dB (cible %.0f)", kPresets[i].id, m.rmsDb, target); worst = std::max (worst, err); }
    }
    std::printf ("niveaux : écart max %.2f dB\n", worst);
    // 2. échantillons : région synthétique (sinus 440 Hz, racine 69) jouée à 220 Hz
    {
        kal::SamplePatch patch; kal::SampleRegion r; r.root = 69; r.sr = 48000; r.frames = 48000; r.data.resize (96000);
        for (int i = 0; i < r.frames; ++i) r.data[(size_t) i * 2] = r.data[(size_t) i * 2 + 1] = 0.5f * (float) std::sin (2 * 3.14159265 * 440 * i / 48000.0);
        patch.regions.push_back (r);
        kal::SynthEngine e; e.prepare (48000, 256); kal::SynthParams p; p.wtOn = false; p.smpOn = true; p.reverb = 0; p.chorus = 0; p.cutoff = 1; p.fEnv = 0; p.levelDb = 0; p.width = 1;
        e.setParams (p); e.setPatch (&patch); e.noteOn (57, 1.f);
        std::vector<float> L (4800), R (4800); e.render (L.data(), R.data(), 4800); e.render (L.data(), R.data(), 4800);
        int zc = 0; for (int i = 1; i < 4800; ++i) if ((L[i - 1] < 0) != (L[i] < 0)) ++zc;
        const double hz = zc / 2.0 / 0.1;
        std::printf ("échantillon transposé : %.0f Hz (attendu 220)\n", hz);
        CHECK (std::abs (hz - 220) < 6, "transposition échantillon %.0f Hz", hz);
        e.setPatch (nullptr); e.render (L.data(), R.data(), 4800);      // échange de patch en cours de jeu
    }
    // 3. granulaire sans échantillon + polyphonie au-delà de 16 voix + mono legato
    {
        kal::SynthEngine e; e.prepare (44100, 512); kal::SynthParams p; p.granOn = true; p.granMix = 0.8f; e.setParams (p);
        std::vector<float> L (512), R (512); bool ok = true; double s = 0;
        for (int n = 0; n < 24; ++n) e.noteOn (40 + n * 2, 0.7f);
        CHECK (e.activeVoices() == 16, "vol de voix : %d voix", e.activeVoices());
        for (int b = 0; b < 200; ++b) { e.render (L.data(), R.data(), 512); for (float x : L) { ok &= std::isfinite (x); s += x * x; } }
        CHECK (ok && s > 1, "granulaire polyphonique muet ou instable");
        e.allNotesOff(); for (int b = 0; b < 400; ++b) e.render (L.data(), R.data(), 512);
        CHECK (e.activeVoices() == 0, "voix non libérées : %d", e.activeVoices());
        p.granOn = false; p.mono = true; p.glideMs = 60; e.setParams (p);
        e.noteOn (60, 0.8f); e.noteOn (64, 0.8f); e.noteOff (64); e.render (L.data(), R.data(), 512);
        CHECK (e.activeVoices() == 1, "mono : %d voix", e.activeVoices());
        e.noteOff (60); for (int b = 0; b < 200; ++b) e.render (L.data(), R.data(), 512);
        CHECK (e.activeVoices() == 0, "mono : voix restante");
    }
    // 4. coût : 8 voix, unisson 4, granulaire, effets, 48 kHz
    {
        kal::SynthEngine e; e.prepare (48000, 256); kal::SynthParams p; p.unison = 4; p.granOn = true; p.chorus = 0.4f; p.delay = 0.3f; p.reverb = 0.5f; p.f24 = true; e.setParams (p);
        for (int n = 0; n < 8; ++n) e.noteOn (48 + n * 3, 0.7f);
        std::vector<float> L (256), R (256);
        auto t = std::chrono::steady_clock::now();
        for (int b = 0; b < 48000 * 5 / 256; ++b) e.render (L.data(), R.data(), 256);
        const double pct = std::chrono::duration<double> (std::chrono::steady_clock::now() - t).count() / 5.0 * 100;
        std::printf ("CPU 8 voix pleines : %.1f %% d'un cœur\n", pct);
        CHECK (pct < 25, "trop lourd : %.1f %%", pct);
    }
    std::printf (fails ? "%d échec(s)\n" : "Tous les tests moteur passent\n", fails);
    return fails ? 1 : 0;
}
