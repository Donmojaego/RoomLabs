#pragma once
// =============================================================================
// Shake It — moteur de basse mono (808, Sub, Moog-style, Pincée).
// C++ pur, sans JUCE : testable hors plugin (tests/EngineTest.cpp).
// Règles temps réel : aucune allocation, aucun verrou après prepare().
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace shk
{
enum class Model : int { Bass808 = 0, Sub = 1, Moog = 2, Pluck = 3 };
enum class DriveType : int { Soft = 0, Hard = 1, Fold = 2 };

struct Params
{
    Model model = Model::Bass808;
    float tune = 0.f;          // demi-tons
    float glideMs = 120.f;     // 0..1000
    bool glideLegatoOnly = true;
    float punch = 0.5f;        // 0..1
    float decay = 1.5f;        // secondes (t60 pendant la tenue)
    float drive = 0.f;         // 0..1
    DriveType driveType = DriveType::Soft;
    float outputDb = 0.f;
    float harm = 0.f;          // 0..1
    float tone = 18000.f;      // Hz
    float reso = 0.f;          // 0..1
    float attackMs = 1.f;
    float releaseMs = 120.f;
};

// Ce que l'interface lit pour animer (écrit par le thread audio, lu ailleurs).
struct Motion
{
    std::atomic<float> energy { 0.f };     // énergie du grave lissée, 0..~1
    std::atomic<int> onsets { 0 };         // compteur d'attaques
    std::atomic<float> glide { 0.f };      // 0 = pas de glide en cours, 1 = début du glide
    std::atomic<int> note { -1 };          // note jouée, -1 au silence
    std::atomic<float> level { 0.f };      // niveau crête de sortie (bloc)
};

class BassEngine
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { prm = p; }

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();

    // Ajoute (mono) dans out[0..n). Écrit aussi la même chose dans outR si non nul.
    void render (float* outL, float* outR, int n);

    bool isSounding() const { return ampEnv > 1.0e-5f || gate; }
    Motion motion;

    // utilitaires de test
    double currentFrequency() const { return curHz; }

private:
    float renderSample();
    void trigger (int note, float velocity, bool legato);
    void setTarget (int note, bool glide);
    float shape (float x) const;
    float ladder (float x, float cutoffHz, float res);

    Params prm;
    double sr = 48000.0, invSr = 1.0 / 48000.0;

    // pile de notes (priorité à la dernière)
    std::array<int, 16> stack {};
    int stackSize = 0;

    bool gate = false;
    float velocity = 1.f;

    // hauteur
    double curSemi = 36.0, startSemi = 36.0, targetSemi = 36.0;
    double glidePos = 1.0, glideStep = 1.0;
    double curHz = 65.4;

    // oscillateurs
    double phase = 0.0;
    double clickPhase = 0.0;
    double tSinceTrigger = 1.0;

    // enveloppes
    float ampEnv = 0.f;
    float attackStart = 0.f;
    bool inAttack = false;
    double attackPos = 0.0;

    // filtres
    float lpState1 = 0.f, lpState2 = 0.f;      // passe-bas Tone (2 pôles)
    std::array<float, 4> ld {};                // ladder Moog
    float dcX = 0.f, dcY = 0.f;                // anti-continu
    float bassLp = 0.f, follower = 0.f;        // énergie du grave

    // corde pincée
    std::vector<float> ks;
    int ksWrite = 0;
    float ksLast = 0.f;
    uint32_t rng = 0x12345678u;

    float outGainSmoothed = 1.f;
};
} // namespace shk
