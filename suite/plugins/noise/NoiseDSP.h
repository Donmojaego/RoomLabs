#pragma once
// =============================================================================
// Noise — générateur de bruit blanc / rose / brun, couleur (pente), niveau RMS, largeur stéréo.
// Moteur pur C++ (testé hors JUCE). Sortie muette au départ, activation et réglages sans clic.
// Le bruit est ajouté au signal d'entrée (le plugin s'insère sur n'importe quelle piste).
// =============================================================================
#include <array>
#include <atomic>
#include <cstdint>

namespace rl::nz
{
struct Params
{
    bool on = false;
    int type = 1;            // 0 blanc, 1 rose, 2 brun
    float color = 0.f;       // -1..+1 : pente supplémentaire (-3..+3 dB/octave autour de 1 kHz)
    float levelDb = -18.f;   // niveau RMS de sortie du bruit (dBFS), plein spectre
    float width = 1.f;       // 0 = mono, 1 = deux canaux décorrélés
    bool keepInput = true;   // le signal de la piste passe (sinon : bruit seul)
};

struct Meter { std::atomic<float> rmsL { 0 }, rmsR { 0 }; std::array<std::atomic<float>, 64> scope {}; };

class Engine
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { prm = p; }
    void process (float* const* io, int numCh, int n);
    Meter meter;

private:
    struct Gen
    {
        uint64_t s = 0x9E3779B97F4A7C15ull;
        float b[7] {};          // rose (Kellet)
        float brown = 0;        // brun
        float tiltZ = 0;        // filtre de couleur
        float msIn = 1, msOut = 1, corr = 1;   // compensation de niveau de la couleur
        float next (int type, float brownLeak);
        float white();
    };
    float tilt (Gen& g, float x);
    Params prm;
    double sr = 48000;
    Gen g1, g2;
    float gain = 0, colorS = 0, widthS = 1, typeFade = 1;
    int curType = 1, pendingType = -1;
    float tiltA = 0, brownLeak = 0, smK = 0, msK = 0;
    int scopePos = 0; float scopeAcc = 0; int scopeN = 0;
};
} // namespace rl::nz
