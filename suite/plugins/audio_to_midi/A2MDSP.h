#pragma once
// =============================================================================
// Audio to MIDI — conversion en temps réel (C++ pur, testée hors JUCE).
// Mode Mélodie : détection de hauteur YIN (monophonique), notes avec vélocité, re-déclenchement sur attaque.
// Mode Batterie : détection d'attaques par bandes → grosse caisse 36, caisse claire 38, charley 42.
// Aucune allocation dans process ; les événements sortent avec leur position dans le bloc.
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rl::a2m
{
struct Params
{
    int mode = 0;               // 0 Mélodie, 1 Batterie
    float threshDb = -45.f;     // seuil de déclenchement
    float minLenMs = 60.f;      // durée minimale d'une note
    int transpose = 0;          // demi-tons
    int lowNote = 28, highNote = 96;
    float velSens = 0.7f;       // 0 = vélocité fixe 100, 1 = suit pleinement le niveau
};

struct Event { int offset; int note; int vel; };   // vel 0 = note off

struct Meter { std::atomic<int> note { -1 }; std::atomic<float> hz { 0 }, conf { 0 }, levelDb { -120 }; };

class Engine
{
public:
    static constexpr int kMaxEvents = 128;
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { prm = p; }
    // analyse (le son n'est pas modifié) ; renvoie le nombre d'événements écrits dans ev
    int process (const float* const* in, int numCh, int n, Event* ev);
    Meter meter;
    double latencyMs() const { return 1000.0 * (W * dec) / sr; }
private:
    void analyseFrame (int offset, Event* ev, int& ne);
    void emit (Event* ev, int& ne, int off, int note, int vel) { if (ne < kMaxEvents) ev[ne++] = { off, note, vel }; }
    Params prm; double sr = 48000; int dec = 2;
    static constexpr int W = 1024, Hop = 128;              // fenêtre / pas (en échantillons décimés)
    std::vector<float> ring; int rpos = 0, hopCnt = 0, decCnt = 0; float decAcc = 0;
    std::vector<float> frame, diff;
    float aa1 = 0, aa2 = 0;                                // anti-repliement avant décimation
    int curNote = -1, candNote = -1, candFrames = 0, offFrames = 0; float prevDb = -120;
    // batterie
    float lpS = 0, hpS = 0, hpX = 0, envL = 0, envM = 0, envH = 0, slowL = 0, slowM = 0, slowH = 0;
    int refractory = 0, drumOff = 0, drumNote = -1, pend = 0; float pL = 0, pM = 0, pH = 0;
};
} // namespace rl::a2m
