#pragma once
// =============================================================================
// African Riddim — moteur de synthèse (C++ pur, testé hors JUCE).
// 16 voix ; chaque voix joue l'instrument courant avec son modèle physique / modal :
//   Pluck     corde pincée (Karplus-Strong + grésillement de chevalet + caisse)
//   Lamella   lamelles (sanza, likembe, mbira) : modes inharmoniques + anneaux qui grésillent
//   Bar       lames frappées (balafon, timbila…) avec calebasse à mirliton ; cloche (agogo) ; tambour à fente (gbi)
//   Drum      membranes (modes de Bessel), tambour parlant, tambour à friction, tambour d'eau
//   Wind      flûte, anches doubles / cornemuse avec bourdon, trompe longue, arc soufflé
//   Bow       vièle à archet (goge, segaba)
//   MouthBow  arc musical : corde frappée dont un résonateur fait chanter les harmoniques
// Aucune allocation dans process().
// =============================================================================
#include "Instruments.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace afr
{
struct Params
{
    int instrument = 25;          // RDC — Likembe
    float volumeDb = 0.f;         // volume général (le niveau de chaque instrument est déjà calibré)
    float attackMs = 0.f;         // attaque ajoutée
    float releaseMs = 300.f;      // relâchement après la note
    int octave = 0;               // -3..+3
    float tune = 0.f;             // demi-tons -12..+12
    float tone = 0.5f;            // couleur 0 sombre .. 1 brillant
    float space = 0.25f;          // réverbération
    float velSens = 0.7f;         // sensibilité au toucher
};

struct Feedback
{
    std::atomic<int> hits { 0 };          // compteur de notes jouées (animation)
    std::atomic<float> lastVel { 0 };
    std::atomic<float> level { 0 };       // niveau de sortie (crête lissée)
    std::atomic<int> voices { 0 };
    std::array<std::atomic<uint8_t>, 128> held {};   // notes tenues (clavier à l'écran)
};

class Engine
{
public:
    Engine();
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void pitchBend (float semis) { bend = semis; }      // ±2 demi-tons
    void modWheel (float v) { mod = v; }                 // vibrato
    void process (float* outL, float* outR, int n);
    Feedback fb;
    static float calibrationDb (int instrument);         // gain de calibration (presets au même niveau)

    static constexpr int kVoices = 16;
    static constexpr int kMaxDelay = 8192;

private:
    struct Biquad { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        inline float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void bandpass (double sr, double f, double q); void lowpass (double sr, double f, double q); void highpass (double sr, double f, double q); void clear() { z1 = z2 = 0; } };
    struct Mode { float f = 0, a = 0, d = 0, c = 0, s = 0, cs = 0, sn = 0, re = 0, im = 0; };   // oscillateur amorti (rotation complexe)

    struct Voice
    {
        bool active = false, held = false, releasing = false;
        int note = 0, inst = 0; float vel = 0, freq = 0, age = 0;
        float env = 0, envTarget = 0, atkInc = 1, relMul = 0.999f, gain = 1, pan = 0;
        // corde
        float dl[kMaxDelay]; int w = 0; float delay = 100, apC = 0, apZ = 0, lpZ = 0, loss = 0.99f, lpA = 0.5f, dcX = 0, dcY = 0;
        float dl2[kMaxDelay]; int w2 = 0; float delay2 = 100, ap2C = 0, ap2Z = 0, lp2Z = 0;
        int exciteLeft = 0; float exLp = 0;
        // modes (lamelles, lames, membranes)
        std::array<Mode, 10> modes; int nModes = 0;
        // souffle / archet
        float phase = 0, phase2 = 0, phaseDr = 0, noiseLp = 0, vib = 0, jitter = 0;
        // tambour
        float pitchEnv = 0, pitchDrop = 0, slap = 0, fric = 0;
        Biquad body, body2, res, buzzBp;
        float buzzEnv = 0;
        uint32_t rng = 1;
        inline float noise() { rng = rng * 1664525u + 1013904223u; return (float) (int32_t) rng * (1.f / 2147483648.f); }
    };

    void startVoice (Voice& v, int note, float vel);
    float renderVoice (Voice& v, float vibLfo);
    float stringStep (Voice& v, float* buf, int& w, float delay, float& apC, float& apZ, float& lpZ, float input);
    double sr = 48000;
    Params prm;
    std::vector<Voice> voices;
    float limG = 1, limRel = 0.0001f;
    float bend = 0, mod = 0, lfo = 0, toneZL = 0, toneZR = 0, levelS = 0, volS = 1;
    int stamp = 0;
    // réverbération (FDN 8 lignes)
    static constexpr int kFdn = 8;
    std::vector<float> fdnBuf[kFdn]; int fdnLen[kFdn] {}, fdnPos[kFdn] {}; float fdnLp[kFdn] {}; float fdnG = 0.8f, fdnDamp = 0.3f;
    std::vector<float> preBuf; int prePos = 0;
};
} // namespace afr
