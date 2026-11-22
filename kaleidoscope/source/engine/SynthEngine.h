#pragma once
// =============================================================================
// Kaleidoscope — moteur sonore hybride (C++ pur, sans JUCE).
// Par voix : 2 oscillateurs wavetable (8 tables à morphing, limitées en bande par octave, unisson),
// sub + bruit, couche granulaire, couche d'échantillons (banques CC0), filtre multimode 12/24 dB
// avec saturation, 2 enveloppes, LFO. Effets : saturation, chorus, écho au tempo, réverbération FDN,
// largeur stéréo, tilt, limiteur. Temps réel : aucune allocation ni verrou dans render().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace kal
{
// ---- banques d'échantillons (chargées hors du fil audio, échangées par pointeur atomique)
struct SampleRegion
{
    int lo = 0, hi = 127, root = 60, vlo = 0, vhi = 127;
    std::vector<float> data;     // stéréo entrelacé
    int frames = 0; double sr = 44100; float gain = 1.f; float tune = 0.f;
    bool loop = false; int loopStart = 0, loopEnd = 0;
};
struct SamplePatch { std::vector<SampleRegion> regions; float releaseMs = 300; bool percussive = false; };

struct SynthParams
{
    // oscillateurs
    bool wtOn = true; int tableA = 0, tableB = 1; float posA = 0.3f, posB = 0.5f, mixB = 0.4f;
    float semiB = 0, fineB = 7; int unison = 2; float detune = 0.2f, spread = 0.6f;
    float sub = 0.2f, noise = 0.f, wtEnv = 0.f, wtLfo = 0.f;
    // filtre
    int fMode = 0; bool f24 = false; float cutoff = 0.7f, reso = 0.2f, fEnv = 0.3f, keytrack = 0.5f, drive = 0.1f;
    // enveloppes (ms / 0..1)
    float aA = 5, aD = 600, aS = 0.7f, aR = 400;
    float fA = 2, fD = 500, fS = 0.3f, fR = 400;
    // LFO
    float lfoRate = 4.f; float lfoVib = 0.f, lfoCut = 0.f, lfoAmp = 0.f;
    // granulaire
    bool granOn = false; float granSize = 80, granDensity = 30, granSpray = 0.3f, granPitch = 0.f, granPos = 0.3f, granMix = 0.5f;
    // échantillons
    bool smpOn = false; float smpLevel = 0.8f, smpTune = 0.f;
    // jeu
    bool mono = false; float glideMs = 0; float bendRange = 2;
    // effets
    float fxDrive = 0.f, chorus = 0.f, delay = 0.f, delayFb = 0.35f; int delayDiv = 2; float reverb = 0.2f, revSize = 0.6f, width = 0.8f, tilt = 0.f, levelDb = -6.f;
};

struct Feedback
{
    std::atomic<float> energy { 0.f }, low { 0.f }, high { 0.f };
    std::atomic<int> noteHits { 0 }, lastNote { 60 }, voices { 0 };
    std::atomic<float> lastVel { 0.f }, level { 0.f };
};

class WaveTables
{
public:
    static constexpr int kTables = 8, kFrames = 16, kSize = 2048, kMips = 9;
    static const WaveTables& get();
    const float* frame (int table, int frame, int mip) const { return data.data() + (((size_t) table * kFrames + (size_t) frame) * kMips + (size_t) mip) * kSize; }
    static const char* name (int t);
private:
    WaveTables();
    std::vector<float> data;
};

class SynthEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const SynthParams& p) { prm = p; }
    void setTempo (double bpm) { tempo = bpm > 1 ? bpm : 120; }
    // le processeur garde le patch vivant (et les anciens quelques secondes) : ici un simple pointeur atomique
    void setPatch (const SamplePatch* p) { patchPtr.store (p, std::memory_order_release); }
    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff();
    void pitchBend (float v) { bend = v; }
    void render (float* L, float* R, int n);
    int activeVoices() const;
    Feedback fb;

private:
    struct Svf { float ic1 = 0, ic2 = 0; void reset() { ic1 = ic2 = 0; } };
    struct Grain { bool on = false; double pos = 0, inc = 1; int age = 0, len = 1; float pan = 0, amp = 1; };
    struct Voice
    {
        bool active = false, released = false; int note = 0; float vel = 1; uint64_t age = 0;
        double hz = 440, hzTarget = 440; float glideCoef = 1;
        std::array<double, 4> phA {}, phB {}; double phSub = 0;
        float aEnv = 0, fEnv = 0; int aStage = 0, fStage = 0;
        Svf f1L, f2L, f1R, f2R; double lfoPh = 0;
        // échantillons
        const SampleRegion* reg = nullptr; double sPos = 0, sInc = 1; bool sDone = true;
        // granulaire
        std::array<Grain, 12> grains {}; double grainAcc = 0;
        // valeurs de contrôle recalculées tous les 16 échantillons
        int ctl = 0; double incA[4] {}, incB[4] {}; double fa1 = 1, fa2 = 0, fa3 = 0, fk = 2; double hzNow = 440; int mip = 0, mipB = 0;
    };
    void startVoice (Voice& v, int note, float vel, bool legato);
    void renderVoice (Voice& v, float& l, float& r, const float* tabMods);
    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (int32_t) rng * (1.f / 2147483648.f); }

    SynthParams prm; double sr = 48000, invSr = 1.0 / 48000, tempo = 120; float bend = 0;
    std::array<Voice, 16> voices {}; uint64_t ageCounter = 0; uint32_t rng = 0x1234567u;
    std::array<int, 16> heldStack {}; int heldCount = 0;
    std::atomic<const SamplePatch*> patchPtr { nullptr }; const SamplePatch* patch = nullptr;
    std::vector<float> grainSrc; double grainSrcHz = 261.6255653; int grainSrcTable = -1; float grainSrcPos = -1; int grainCooldown = 0;
    // effets
    std::vector<float> chL, chR, dlL, dlR; int chW = 0, dlW = 0; double chPh = 0; float dlTimeSm = 0;
    struct Fdn { std::array<std::vector<float>, 8> d; std::array<int, 8> w {}; std::array<float, 8> lp {}; double modPh = 0; } fdn;
    float tiltLo = 0, tiltLoR = 0, follow = 0, lowF = 0, highF = 0, limGain = 1, levelSm = 0.5f;
    void rebuildGrainSource();
    struct EnvC { float aInc = 1, dC = 0, rC = 0; } envA, envF;
    double detRatio[4] { 1, 1, 1, 1 }; float uniNorm = 1, panL[4] { 0.7f, 0.7f, 0.7f, 0.7f }, panR[4] { 0.7f, 0.7f, 0.7f, 0.7f };
};
} // namespace kal
