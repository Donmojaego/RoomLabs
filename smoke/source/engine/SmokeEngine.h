#pragma once
// =============================================================================
// Smoke — moteur (C++ pur, sans JUCE : testable hors plugin).
// R&B, hip-hop, house : Rhodes, piano feutré, nappes, risers, subs et basses.
// Modules : Mod (dérive de hauteur + chorus), Tone (filtre), Space (réverbération),
// Dream (longues queues), Clean (moins de saturation douce). Pas de couche de bruit :
// le souffle vient de l'attaque, du filtrage et du mouvement stéréo.
// Règles temps réel : aucune allocation ni verrou après prepare().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace smk
{
enum class Model : int { Rhodes = 0, Felt, Pad, Riser, Sub, HouseBass, Count };

struct Params
{
    Model model = Model::Rhodes;
    float tone = 0.7f;        // 0..1 : ouverture du filtre
    float space = 0.3f;       // réverbération
    float mod = 0.3f;         // dérive de hauteur + chorus
    float dream = 0.2f;       // longueur des queues
    float clean = 0.5f;       // 0 = saturation douce, 1 = propre
    float levelDb = -6.f;
    float attackMs = 5.f, decayMs = 1500.f, sustain = 0.6f, releaseMs = 500.f;
    float bright = 0.5f;      // couleur de l'instrument
    float tremolo = 0.f;      // Rhodes : vibrato d'amplitude stéréo
    float riseS = 4.f;        // Riser : durée de la montée
    float glideMs = 80.f;     // basses mono
    int chord = 0;            // 0 aucun, 1 m9, 2 maj9, 3 m11, 4 9, 5 maj7
    float tune = 0.f;
};

struct Feedback
{
    std::atomic<float> energy { 0.f };     // niveau lissé (braise, densité de fumée)
    std::atomic<float> intensity { 0.f };  // intensité des effets (yeux qui rougissent)
    std::atomic<int> noteHits { 0 };
    std::atomic<float> lastVel { 0.f };
    std::atomic<int> held { 0 };           // touches tenues (fumée qui monte) ; 0 = relâché (dissipation)
    std::atomic<int> voices { 0 };
    std::atomic<float> level { 0.f };
};

class SmokeEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void render (float* outL, float* outR, int n);
    Feedback fb;
    int activeVoices() const;
    static bool isMono (Model m) { return m == Model::Sub || m == Model::HouseBass; }

private:
    struct Voice
    {
        bool active = false, released = false; int note = 0, key = 0; float vel = 1;
        double hz = 440, t = 0, ph[8] {}, mph = 0, driftPh[2] {}, panPh = 0;
        float env = 0; int stage = 0; float partEnv[8] {}; float partAmp[8] {}; float modEnv = 1, tineEnv = 1;
        float ic1 = 0, ic2 = 0, a1 = 1, a2 = 0, a3 = 0; unsigned tick = 0;
        float pan = 0; uint64_t age = 0;
        double glideFrom = 0, glideTo = 0, glidePos = 1;
    };
    void startVoice (int note, int key, float vel);
    void renderVoice (Voice& v, float& l, float& r);

    Params prm;
    double sr = 48000, invSr = 1.0 / 48000;
    std::array<Voice, 12> voices {};
    uint64_t ageCounter = 0; uint32_t rng = 0xC0FFEEu;
    std::array<int, 16> held {}; int heldCount = 0;
    float aInc = 0, dCoef = 0, rCoef = 0, kRhodesMod = 1, kRhodesTine = 1, kBassEnv = 1;
    std::array<float, 8> feltCoef {};

    // chorus stéréo
    std::vector<float> chL, chR; int chW = 0; double chLfo = 0;
    // réverbération (type Freeverb) + pré-délai
    struct Comb { std::vector<float> b; int i = 0; float store = 0; };
    struct AllPass { std::vector<float> b; int i = 0; };
    std::array<Comb, 8> combL, combR; std::array<AllPass, 4> apL, apR;
    std::vector<float> pre; int preW = 0;
    float toneL1 = 0, toneL2 = 0, toneR1 = 0, toneR2 = 0;
    float follower = 0, wetFollow = 0, levelSm = 0.5f, spaceSm = 0.3f, dreamSm = 0.2f, toneSm = 0.7f, toneA = 0.5f;
};
} // namespace smk
