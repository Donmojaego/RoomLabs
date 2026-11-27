#pragma once
// =============================================================================
// Loop Remix Lab — remixeur en temps réel calé sur le tempo de l'hôte.
// 16 pas (double-croches), chaque pas reçoit un effet parmi 16 (répétition, roll, reverse, ralenti,
// tape stop, scratch, gate, mute, crush, filtre, ring, écho, pan, pitch +12, drift…).
// 2 LFO synchronisés (5 formes) vers filtre / volume / pan / crush / ring / résonance.
// Fondu de 3 ms entre pas (pas de clic), aucune allocation dans process, rendu indépendant des blocs.
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rl::rx
{
enum Fx { Thru, Repeat, Roll, Reverse, Half, TapeStop, Scratch, Gate, Mute, Crush, Sweep, Ring, Echo, Pan, PitchUp, Drift, NumFx };

struct Lfo { int shape = 0; int rate = 3; float depth = 0; int dest = 0; };   // rate : index de kRates ; dest : 0 filtre 1 volume 2 pan 3 crush 4 ring 5 résonance
struct Params
{
    std::array<int, 16> steps {};
    float cutoff = 20000.f, reso = 0.2f, mix = 1.f, outDb = 0.f;
    Lfo lfo[2];
};

struct Meter { std::atomic<int> step { 0 }; std::atomic<float> lfo1 { 0 }, lfo2 { 0 }; std::array<std::atomic<float>, 64> wave {}; };

class Engine
{
public:
    static constexpr double kRates[7] = { 16, 8, 4, 2, 1, 0.5, 0.25 };   // en noires : 4 mesures … 1/16
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { prm = p; }
    void setTransport (double bpm, double ppqAtBlockStart, bool playing);
    void process (float* const* io, int numCh, int n);
    Meter meter;
private:
    float read (int ch, double pos) const;
    void render (int fx, int64_t start, double t, double L, float& l, float& r, float& g) const;
    double lfoValue (int k, double ppq);
    Params prm; double sr = 48000, bpm = 120, ppq = 0; bool hostPlay = false;
    std::vector<float> buf[2]; int size = 0; int64_t abs = 0;
    int curIdx = -1, curFx = 0, prevFx = 0; int64_t stepStart = 0, prevStart = 0; float fade = 0;
    float gS = 1, crushS = 0, sweepS = 0, ringS = 0, panS = 0;
    float ic1[2] {}, ic2[2] {}, hold[2] {}; int holdN = 0; double ringPh = 0;
    double shTime[2] { -1, -1 }; float shVal[2] {}; uint32_t seed = 12345;
    float envAcc = 0; int envN = 0, wavePos = 0;
};
} // namespace rl::rx
