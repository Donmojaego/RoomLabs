#pragma once
// =============================================================================
// Noise Tools — restauration temps réel : De-clip → De-click → Hum → Denoise.
// Moteur pur C++ (testable sans JUCE). Latence fixe = HOP (contexte des étapes temporelles) + N (STFT),
// identique que les traitements soient actifs ou non (le DAW compense une seule fois).
// =============================================================================
#include <array>
#include <atomic>
#include <complex>
#include <cstdint>
#include <vector>

namespace rl::nt
{
struct Params
{
    bool dnOn = true, dcOn = true, dpOn = false, bypass = false, compare = false, learn = false;
    float dnAmount = 12.f;     // dB de réduction maximale (plancher du gain)
    float dnSens = .5f;        // 0..1 : surestimation du bruit (plus = plus agressif)
    int hum = 0;               // 0 arrêt, 1 = 50 Hz, 2 = 60 Hz
    float dcSens = .5f;        // 0..1 : seuil des clics (plus = plus de détections)
    float dcLenMs = 1.5f;      // longueur maximale d'un clic réparé
    float dpSens = .5f;        // 0..1 : tolérance du plateau écrêté
    float dpMarginDb = -3.f;   // marge de sortie quand De-clip est actif (les crêtes reconstruites dépassent)
    float outDb = 0.f;
};

struct Meter
{
    std::atomic<float> inL { 0 }, inR { 0 }, outL { 0 }, outR { 0 }, reductionDb { 0 }, clicksPerSec { 0 }, clipsPerSec { 0 }, humDb { 0 };
    std::array<std::atomic<float>, 32> specIn {}, specOut {}, noise {};
};

class Engine
{
public:
    static constexpr int N = 2048, HOP = 512, BINS = N / 2 + 1, AR = 16;
    static constexpr int latency() { return HOP + N; }

    void prepare (double sampleRate, int channels);
    void reset();
    void setParams (const Params& p) { prm = p; }
    void process (float* const* io, int numCh, int n);
    Meter meter;
    double sr = 48000;

    // statistiques pour les tests
    long totalClicks = 0, totalClipRuns = 0;

private:
    struct Chan
    {
        // étapes temporelles
        std::vector<char> rep;               // échantillons déjà reconstruits par le De-clip
        std::vector<float> inFifo, ctx;      // ctx = [prev | cur | next] (3 x HOP)
        int inPos = 0;
        // hum (biquads en série, état)
        std::array<std::array<double, 4>, 8> hum {};
        // STFT
        std::vector<float> stIn, stOut; int stPos = 0;
        std::vector<float> prevP, noiseP, prevG, gSmooth, learnAcc; int learnN = 0;
        std::vector<std::complex<float>> spec;
        // sortie
        std::vector<float> outFifo; int outRead = 0, outWrite = 0;
        // dry retardé (bypass / comparaison)
        std::vector<float> dry; int dryPos = 0;
        float rmsIn = 0, rmsOut = 0;
    };
    void frameTime (Chan& c);                 // traite c.ctx (cur) et pousse HOP échantillons vers la STFT
    void stftFrame (Chan& c);
    void declip (float* x, char* rep, int len, int a, int b);
    bool arCoefs (const float* x, int len, double* a);
    static constexpr int MAXGAP = 256;
    std::vector<double> lsL, lsV; std::vector<char> lsU;
    int declick (float* x, int len, int a, int b);
    void humFilter (Chan& c, float* x, int n);
    void fft (std::complex<float>* a, bool inv);

    Params prm;
    std::vector<Chan> ch;
    std::vector<float> win, tmp, tmp2, cums;
    std::vector<std::complex<float>> tw;
    std::vector<int> bitrev;
    std::array<std::array<double, 5>, 8> humCoef {};
    int humMode = -1;
    float xfBypass = 0, xfCompare = 0, matchGain = 1, outGain = 1, marginGain = 1;
    int clickCount = 0, clipCount = 0, statFrames = 0;
    std::vector<float> bandIn, bandOut, bandNoise;
};
} // namespace rl::nt
