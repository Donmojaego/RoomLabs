#pragma once
// =============================================================================
// Ma petite soupe ouselem — moteur (C++ pur, sans JUCE : testable hors plugin).
// Un sachet = un instrument + un module d'effet calé sur le tempo.
//  Instruments : violon, pad, piano, cloche/gouttes, cordes, texture, basse sourde, tambour « touc »
//  Effets      : reverse, stutter, répétitions, découpage, étirement, filtre modulé, grain delay
//  + interrupteurs Reverse et Stutter, Tone, Damp, Space, Mix.
// Règles temps réel : aucune allocation ni verrou après prepare().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace soup
{
// 8 à 14 : instruments d'inspiration asiatique et orientale (banque « Mon petit ramen », v0.1.1)
enum class Inst : int { Violin = 0, Pad, Keys, Bell, Strings, Texture, Bass, Drum, Koto, Shamisen, Erhu, Shakuhachi, Gamelan, Oud, Taiko, Count };
enum class Fx : int { None = 0, Reverse, Stutter, Repeat, Slicer, Stretch, FilterMod, GrainDelay, Count };

struct Params
{
    Inst inst = Inst::Violin;
    Fx fx = Fx::GrainDelay;
    bool reverseOn = false, stutterOn = false;
    int division = 2;            // 0 1/2, 1 1/4, 2 1/8, 3 1/16, 4 1/32, 5 1/4T, 6 1/8T, 7 1/16T
    float mix = 0.5f;            // part de l'effet
    float feedback = 0.4f;       // répétitions, grains
    float stretch = 0.5f;        // 0 = vitesse normale, 1 = très étiré
    float tone = 0.f;            // -1..1
    float damp = 0.4f;           // amortit les aigus de la réverbération et des répétitions
    float space = 0.3f;          // réverbération
    float levelDb = -6.f;
    float attackMs = 20.f, releaseMs = 600.f, sustain = 0.8f, decayMs = 800.f;
    float brightness = 0.5f;     // 0..1 couleur de l'instrument
    float tune = 0.f;
    bool arpOn = false; int arpMode = 0; int arpOct = 1;
};

struct Feedback
{
    std::atomic<float> energy { 0.f };   // niveau lissé (surface du bol)
    std::atomic<float> wet { 0.f };      // activité de l'effet (vapeur, tourbillon)
    std::atomic<int> noteHits { 0 };
    std::atomic<float> beat { 0.f };     // position dans la division (0..1)
    std::atomic<int> voices { 0 };
    std::atomic<float> level { 0.f };
};

class SoupEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p);
    // tempo de l'hôte ; ppq = position en noires au début du bloc (-1 si à l'arrêt)
    void setTransport (double bpm, double ppqAtBlockStart);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void render (float* outL, float* outR, int n);
    Feedback fb;

    int activeVoices() const;
    double divisionSamples() const;

private:
    struct Voice
    {
        bool active = false, released = false; int note = 0, key = 0; float vel = 1.f;
        double hz = 440, t = 0, ph[3] {}, mph = 0, part[3] {};
        float env = 0.f; int stage = 0; float partEnv[3] { 1, 1, 1 }, modEnv = 1.f;
        float ic1 = 0, ic2 = 0, a1 = 1, a2 = 0, a3 = 0; unsigned tick = 0;
        float bp[3][2] {};   // résonances de caisse (violon)
        double hp[8] {}; float he[8] {}; float buzz = 0;   // cordes pincées / gongs (instruments asiatiques et orientaux)
        uint64_t age = 0;
    };
    void startVoice (int note, int key, float vel);
    float renderVoice (Voice& v);
    float noise();
    float processFx (Fx type, float x, float& wetR, int stage);
    float reverseStage (float x, int which);
    float stutterStage (float x, int which);

    Params prm;
    double sr = 48000, invSr = 1.0 / 48000, bpm = 120;
    double ppq = -1, internalBeats = 0;     // horloge musicale (en noires)
    double beatPos = 0;                     // position courante (noires) pour l'échantillon en cours
    std::array<Voice, 12> voices {};
    uint64_t ageCounter = 0; uint32_t rng = 0x1234567u;
    std::array<int, 16> held {}; int heldCount = 0;
    double arpPhaseDiv = -1; int arpIdx = -1;

    // historique pour les effets (mono) : 8 s
    std::vector<float> hist; int hw = 0;
    // répétitions (stéréo)
    std::vector<float> dlL, dlR; int dw = 0; float dLp = 0, dRp = 0;
    // reverse / stutter : un historique par étage
    std::vector<float> revBuf[2], stuBuf[2]; int revW[2] {}, stuW[2] {};
    // étirement
    double strRead = 0; double strGrainPos[2] {}; int strGrainAge[2] {};
    // grain delay
    struct Grain { bool on = false; double pos = 0, rate = 1; int age = 0, len = 0; float panL = 1, panR = 1; };
    std::array<Grain, 8> grains {}; int grainClock = 0;
    // filtre modulé
    float fic1 = 0, fic2 = 0;
    // réverbération (type Freeverb)
    struct Comb { std::vector<float> b; int i = 0; float store = 0; };
    struct AllPass { std::vector<float> b; int i = 0; };
    std::array<Comb, 8> combL, combR; std::array<AllPass, 4> apL, apR;
    float toneLow = 0, follower = 0, wetFollower = 0, levelSm = 0.5f, mixSm = 0.5f, spaceSm = 0.3f;
    float aInc = 0, dCoef = 0, rCoef = 0;
};
} // namespace soup
