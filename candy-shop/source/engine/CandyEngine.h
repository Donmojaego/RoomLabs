#pragma once
// =============================================================================
// Candy Shop — moteur de son (C++ pur, sans JUCE : testable hors plugin).
//  - instrument polyphonique 16 voix : marimba (modal), synthé soustractif, FM (cloches)
//  - accords (« Carrés fruités ») et arpégiateur calé au tempo (« Lacets acidulés »)
//  - boîte à rythmes synthétisée : notes 24 à 35 (ou canal MIDI 10)
//  - chorus + réverbération (« Space »)
// Règles temps réel : aucune allocation ni verrou après prepare().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace cdy
{
enum class Algo : int { Mallet = 0, Synth = 1, Fm = 2 };

struct Params
{
    Algo algo = Algo::Synth;
    int wave = 2;              // 0 sinus, 1 triangle, 2 dent de scie, 3 carré
    float detune = 8.f;        // centièmes entre voix d'unisson
    int unison = 2;            // 1..3
    float fmRatio = 3.5f, fmAmount = 0.f, fmDecay = 0.4f;
    float noise = 0.f;         // 0..1
    float pitchDrop = 0.f;     // demi-tons au début de la note
    float tune = 0.f;          // demi-tons
    float attackMs = 2.f, decayMs = 300.f, sustain = 0.7f, releaseMs = 250.f;
    float tone = 0.f;          // -1 sombre .. +1 brillant
    float cutoff = 8000.f, reso = 0.1f, fenv = 0.3f, fdecayMs = 250.f;
    float space = 0.2f, chorus = 0.f, levelDb = -6.f, velSens = 0.6f;
    int chord = 0;             // 0 aucun, 1 majeur, 2 mineur, 3 maj7, 4 m7, 5 sus4, 6 quinte, 7 add9
    bool arpOn = false;
    int arpRate = 3;           // 0 1/4, 1 1/8, 2 1/8T, 3 1/16, 4 1/16T, 5 1/32
    int arpMode = 0;           // 0 montant, 1 descendant, 2 aller-retour, 3 hasard, 4 ordre joué
    int arpOct = 1;            // 1..3
    float arpGate = 0.6f;      // 0.1..1
    int kit = 0;               // 0..4
    float drumLevelDb = -4.f;
};

// Ce que l'interface lit (écrit par le thread audio).
struct Feedback
{
    std::atomic<float> energy { 0.f };     // niveau lissé de l'instrument (lueur du bonbon)
    std::atomic<int> noteHits { 0 };       // compteur de notes jouées (instrument)
    std::atomic<int> drumHits[3] {};       // tam-tam, tambour, claves (lueur des percussions)
    std::atomic<int> voices { 0 };         // voix actives
    std::atomic<float> level { 0.f };      // crête du bloc
    std::atomic<int> arpStep { 0 };
};

constexpr int kDrumLow = 24, kDrumHigh = 35, kDrumChannel = 10;

class CandyEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p);
    void setTempo (double bpm) { if (bpm > 20.0 && bpm < 400.0) tempo = bpm; }

    // channel 1..16 ; canal 10 ou notes 24..35 = boîte à rythmes
    void noteOn (int channel, int note, float velocity);
    void noteOff (int channel, int note);
    void allNotesOff();

    void render (float* outL, float* outR, int n);
    Feedback fb;

    // tests
    int activeVoices() const;
    static bool isDrum (int channel, int note) { return channel == kDrumChannel || (note >= kDrumLow && note <= kDrumHigh); }

private:
    struct Voice
    {
        bool active = false, released = false;
        int note = 0, key = 0;          // key = touche qui a déclenché (accords/arpège)
        float vel = 1.f;
        double hz = 440.0, t = 0.0;
        double ph[3] {}, modPh = 0.0, partPh[3] {};
        float env = 0.f; int stage = 0; // 0 attaque, 1 decay, 2 sustain, 3 release
        float fenvLevel = 1.f;
        float ic1 = 0.f, ic2 = 0.f;     // filtre SVF
        float a1 = 1.f, a2 = 0.f, a3 = 0.f; unsigned filterTick = 0;
        float partEnv[3] {};
        uint64_t age = 0;
        float pan = 0.f;
    };
    struct Drum
    {
        bool active = false; int slot = 0; float vel = 1.f; double t = 0.0, ph = 0.0, ph2 = 0.0;
        float bp1 = 0.f, bp2 = 0.f, hp = 0.f, lastN = 0.f;
    };

    void startVoice (int note, int key, float vel);
    void releaseKey (int key);
    void arpNoteOn (int note, float vel);
    void advanceArp();
    void renderVoice (Voice& v, float& l, float& r);
    float renderDrum (Drum& d);
    float noise();

    Params prm;
    double sr = 48000.0, invSr = 1.0 / 48000.0, tempo = 120.0;
    std::array<Voice, 16> voices {};
    std::array<Drum, 8> drums {};
    uint64_t ageCounter = 0;
    uint32_t rng = 0x9E3779B9u;

    // touches tenues (pour accords et arpège)
    std::array<int, 16> held {}; std::array<float, 16> heldVel {}; int heldCount = 0;
    // arpège
    double arpPhase = 0.0; int arpIndex = -1, arpDir = 1, arpCurrent = -1; bool arpWasRunning = false;
    std::array<int, 64> arpSeq {}; int arpSeqLen = 0;

    // coefficients par bloc
    float aInc = 0.f, dCoef = 0.f, rCoef = 0.f, fdCoef = 0.f, modDecCoef = 0.f;

    // effets
    std::vector<float> chorusBuf; int chorusW = 0; double chorusLfo = 0.0;
    struct Comb { std::vector<float> b; int i = 0; float store = 0.f; };
    struct AllPass { std::vector<float> b; int i = 0; };
    std::array<Comb, 8> combL, combR; std::array<AllPass, 4> apL, apR;
    float levelSm = 0.5f, spaceSm = 0.f, follower = 0.f, toneLow = 0.f;
};
} // namespace cdy
