#pragma once
// =============================================================================
// Windup Dogs — moteur (C++ pur, sans JUCE : testable hors plugin).
// Instruments jouets : boîte à musique, piano jouet, clochettes, fanfare, tambour.
// Mécanisme : clé de remontage (bruit de cliquet), ressort qui se détend pendant le jeu
// (la hauteur baisse un peu, le tic-tac ralentit), tic-tac calé sur le tempo de l'hôte.
// Effets : Tone, Méca (bruits mécaniques), Wobble (pleurage), Écho (au tempo), Espace.
// Règles temps réel : aucune allocation ni verrou après prepare().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace wd
{
enum class Inst : int { MusicBox = 0, ToyPiano, Bells, Fanfare, Drum, Count };

struct Params
{
    Inst inst = Inst::MusicBox;
    float tone = 0.75f;        // ouverture du filtre
    float meca = 0.35f;        // bruits mécaniques (picots, marteaux, cliquetis)
    float wobble = 0.15f;      // pleurage / scintillement de hauteur
    float echo = 0.15f;        // écho au tempo (croche pointée, ping-pong)
    float space = 0.3f;        // réverbération
    float levelDb = -6.f;
    float attackMs = 1.f, decayMs = 2500.f, sustain = 0.f, releaseMs = 600.f;
    float bright = 0.5f;       // couleur (partiels aigus)
    float tune = 0.f;
    bool tickOn = false; int tickDiv = 1;   // 0 = 1/4, 1 = 1/8, 2 = 1/16, 3 = 1/8 triolet
    float tickLevel = 0.4f, swing = 0.f;
    bool springOn = true; float springTime = 40.f;   // secondes de jeu pour détendre tout le ressort
};

struct Feedback
{
    std::atomic<float> energy { 0.f };
    std::atomic<int> noteHits { 0 };
    std::atomic<int> lastNote { 60 };
    std::atomic<float> lastVel { 0.f };
    std::atomic<int> held { 0 };
    std::atomic<int> voices { 0 };
    std::atomic<float> tension { 1.f };     // ressort : 1 = remonté, 0 = détendu
    std::atomic<int> ticks { 0 };           // nombre de tic-tac joués
    std::atomic<int> winds { 0 };           // nombre de remontages
    std::atomic<float> beat { 0.f };        // position dans le temps (0..1)
    std::atomic<float> level { 0.f };
};

class WindupEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p);
    void setTransport (double bpm, double ppq);      // ppq < 0 : hôte à l'arrêt
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void wind();                                     // remonter le ressort (clé)
    void render (float* outL, float* outR, int n);
    int activeVoices() const;
    float tension() const { return tens; }
    void setTension (float t) { tens = t < 0 ? 0 : (t > 1 ? 1 : t); }
    Feedback fb;

private:
    struct Svf { float ic1 = 0, ic2 = 0, a1 = 1, a2 = 0, a3 = 0, k = 1;
        void set (double hz, double q, double sr);
        float bp (float x) { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v1; }
        float lp (float x) { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v2; } };
    struct Voice
    {
        bool active = false, released = false; int note = 0, key = 0; float vel = 1; Inst inst = Inst::MusicBox;
        double hz = 440, t = 0; float pan = 0; uint64_t age = 0;
        int np = 0; double ratio[6] {}, ph[6] {}; float amp[6] {}, env[6] {}, coef[6] {};
        float aenv = 0; int stage = 0;                       // 0 attaque, 1 décroissance, 2 tenue, 3 relâchement
        // fanfare
        double vph = 0; Svf f1, f2, lp;
        // tambour
        int drum = 0; double dph = 0, f0 = 200, f1hz = 100; float pitchEnv = 1, pitchCoef = 1, toneEnv = 0, toneCoef = 1, noiseEnv = 0, noiseCoef = 1, toneAmp = 1, noiseAmp = 0, rattle = 0; Svf nf, nf2; int clicks = 0; double clickT = 0, clickGap = 0;
    };
    struct Click { bool on = false; int delay = 0; float env = 0, coef = 0.99f, amp = 0, pan = 0; Svf a, b; float mix = 0.5f; };
    void startVoice (int note, int key, float vel);
    void renderVoice (Voice& v, float& l, float& r, double pitchMul);
    void addClick (double hz, double hz2, float amp, float ms, float pan, int delay = 0);
    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (int32_t) rng * (1.f / 2147483648.f); }

    Params prm;
    double sr = 48000, invSr = 1.0 / 48000, bpm = 120, ppq = -1, freeBeats = 0;
    std::array<Voice, 16> voices {};
    std::array<Click, 24> clicks {};
    uint64_t ageCounter = 0; uint32_t rng = 0x9E3779B9u;
    std::array<int, 16> held {}; int heldCount = 0;
    // mécanisme
    float tens = 1.f, windTarget = -1.f; int windClicks = 0; double windNext = 0; long long lastStep = -1; int tickParity = 0;
    double wobPh1 = 0, wobPh2 = 0, rattleAcc = 0;
    // effets
    std::vector<float> dlL, dlR; int dlW = 0;
    struct Comb { std::vector<float> b; int i = 0; float store = 0; };
    struct AllPass { std::vector<float> b; int i = 0; };
    std::array<Comb, 8> combL, combR; std::array<AllPass, 4> apL, apR;
    float toneL1 = 0, toneL2 = 0, toneR1 = 0, toneR2 = 0, toneSm = 0.75f, toneA = 0.5f;
    float follower = 0, levelSm = 0.5f, echoSm = 0, spaceSm = 0, dlTimeSm = 0;
    float aInc = 0, dCoef = 0, rCoef = 0;
};
} // namespace wd
