#pragma once
// =============================================================================
// Reverb — réverbération algorithmique Room / Hall / Plate (C++ pur, testée hors JUCE).
// Réseau de 8 lignes à retard (FDN, matrice de Householder), diffusion d'entrée, premières réflexions,
// amortissement des aigus dans la boucle, coupe-bas sur l'effet, largeur stéréo.
// Changement de type / taille : fondu enchaîné entre deux moteurs (pas de clic).
// Bypass : l'entrée de la réverbération est coupée mais la queue continue de s'éteindre.
// Rendu identique quelle que soit la taille des blocs (aucun calcul « par bloc »), aucune allocation.
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rl::rv
{
struct Params
{
    int type = 1;              // 0 Room, 1 Hall, 2 Plate
    float size = 0.6f;         // 0..1
    float decay = 2.4f;        // secondes (RT60)
    float predelayMs = 20.f;   // 0..250
    float dampHz = 7000.f;     // 1000..20000
    float lowCutHz = 120.f;    // 20..600 (sur l'effet)
    float mix = 0.3f;          // 0..1
    float width = 1.f;         // 0..1
    bool bypass = false;       // la queue continue
};

struct Meter { std::atomic<float> inL { 0 }, inR { 0 }, wetL { 0 }, wetR { 0 }; std::array<std::atomic<float>, 48> decayEnv {}; };

class Tank
{
public:
    void prepare (double sr);
    void configure (int type, float size, double sr);   // longueurs (hors fil audio quand c'est possible : seulement des indices)
    void clear();
    void process (float inL, float inR, float& outL, float& outR, float decay, float damp, float width);
private:
    static constexpr int N = 8, AP = 4, ER = 8;
    std::vector<float> line[N]; int len[N] {}, pos[N] {}; float lp[N] {};
    std::vector<float> ap[AP]; int apLen[AP] {}, apPos[AP] {};
    std::vector<float> erBuf; int erPos = 0; int erTap[ER] {}; float erGain[ER] {}; float erAmt = 0;
    double sr = 48000, modPh = 0; float apGain = 0.6f; int maxLen = 0;
    uint32_t seed = 1;
};

class Engine
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { prm = p; }
    void process (float* const* io, int numCh, int n);
    Meter meter;
private:
    Params prm; double sr = 48000;
    Tank tank[2]; int active = 0; float xf = 1.f; int curType = -1; float curSize = -1;
    bool pending = false; int pType = 0; float pSize = 0;
    std::vector<float> pre; int prePos = 0;
    float decayS = 2.4f, dampS = 7000, mixS = .3f, widthS = 1, lowS = 120, predS = 20, inGate = 1;
    float hpZL = 0, hpZR = 0, hpXL = 0, hpXR = 0;
    float accIn = 0, accWet = 0; int accN = 0, envPos = 0;
    float smK = 0;
};
} // namespace rl::rv
