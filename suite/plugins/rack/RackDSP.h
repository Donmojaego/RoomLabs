#pragma once
// =============================================================================
// 7 Room Rack — chaîne de 3 emplacements qui accueillent les modules de la suite
// (Noise, Reverb, Loop Remix Lab). Chaque emplacement : module, ON, 3 macros, MIX.
// Changement de module en fondu (20 ms), ON/OFF sans clic, la queue de la Reverb continue en OFF.
// =============================================================================
#include "../noise/NoiseDSP.h"
#include "../reverb/ReverbDSP.h"
#include "../loop_remix_lab/RemixDSP.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rl::rack
{
enum Module { Empty, Noise, Reverb, Remix, NumModules };
struct Slot { int module = Empty; bool on = true; float a = 0.5f, b = 0.5f, c = 0.5f, mix = 1.f; };
struct Params { std::array<Slot, 3> slot {}; float outDb = 0.f; };
struct Meter { std::array<std::atomic<float>, 3> lvl {}; std::atomic<float> out { 0 }; };

// motif généré pour Loop Remix Lab dans le rack : style (0..11) + densité, déterministe
void remixPattern (int style, float density, std::array<int, 16>& steps);

class Engine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p) { prm = p; }
    void setTransport (double bpm, double ppq, bool playing) { tBpm = bpm; tPpq = ppq; tPlay = playing; }
    void process (float* const* io, int numCh, int n);
    Meter meter;
private:
    struct State
    {
        int active = Empty; float g = 0; float onS = 0; int lastStyle = -1; float lastDens = -1; std::array<int, 16> steps {};
        rl::nz::Engine noise; rl::rv::Engine reverb; rl::rx::Engine remix;
    };
    void runModule (int k, State& s, float* const* io, int numCh, int n);
    Params prm; double sr = 48000, tBpm = 120, tPpq = 0; bool tPlay = false;
    std::array<State, 3> st;
    std::vector<float> dry[2], wet[2];
};
} // namespace rl::rack
