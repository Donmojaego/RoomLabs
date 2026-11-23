#include "NoiseDSP.h"
#include <algorithm>
#include <cmath>

namespace rl::nz
{
namespace { constexpr double PI = 3.14159265358979323846; }

// facteurs pour que chaque couleur sorte au même niveau RMS (mesurés sur 60 s, cf. tests)
static constexpr float kNorm[3] = { 1.7320508f /* uniforme ±1 → RMS 1 */, 0.566f, 0.0625f };

void Engine::prepare (double s)
{
    sr = s;
    brownLeak = (float) std::exp (-2 * PI * 5.0 / sr);          // pente -6 dB/oct au-dessus de 5 Hz (pas de dérive continue)
    tiltA = (float) std::exp (-2 * PI * 1000.0 / sr);           // pivot de la couleur : 1 kHz
    smK = (float) (1 - std::exp (-1.0 / (0.03 * sr)));          // lissages 30 ms
    reset();
}

void Engine::reset()
{
    g1 = Gen {}; g2 = Gen {}; g2.s = 0xD1B54A32D192ED03ull;
    gain = 0; colorS = prm.color; widthS = prm.width; curType = prm.type; pendingType = -1; typeFade = 1;
}

float Engine::Gen::white()
{
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;                       // xorshift64
    return (float) ((double) (s >> 11) * (1.0 / 9007199254740992.0) * 2.0 - 1.0);
}

float Engine::Gen::next (int type, float leak)
{
    const float w = white();
    if (type == 0) return w * kNorm[0];
    if (type == 1)
    {   // filtre rose de Paul Kellet (précision ±0,05 dB au-dessus de 9 Hz)
        b[0] = 0.99886f * b[0] + w * 0.0555179f; b[1] = 0.99332f * b[1] + w * 0.0750759f;
        b[2] = 0.96900f * b[2] + w * 0.1538520f; b[3] = 0.86650f * b[3] + w * 0.3104856f;
        b[4] = 0.55000f * b[4] + w * 0.5329522f; b[5] = -0.7616f * b[5] - w * 0.0168980f;
        const float p = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f; b[6] = w * 0.115926f;
        return p * kNorm[1];
    }
    brown = leak * brown + w;
    return brown * kNorm[2];
}

// couleur : mélange d'un passe-bas et d'un passe-haut 1er ordre à 1 kHz → pente douce ±3 dB/oct,
// niveau global conservé (le gain est compensé par la puissance de chaque branche)
float Engine::tilt (Gen& g, float x)
{
    g.tiltZ = tiltA * g.tiltZ + (1 - tiltA) * x;
    const float lo = g.tiltZ, hi = x - lo, c = colorS;
    const float wl = 1 - c, wh = 1 + c;                            // c=-1 : sombre, c=+1 : brillant
    return (lo * wl + hi * wh) / std::sqrt (0.5f * (wl * wl + wh * wh));
}

void Engine::process (float* const* io, int numCh, int n)
{
    const float target = prm.on ? std::pow (10.f, prm.levelDb / 20.f) : 0.f;
    if (prm.type != curType && pendingType != prm.type) { pendingType = prm.type; }
    double accL = 0, accR = 0;
    for (int i = 0; i < n; ++i)
    {
        gain += smK * (target - gain);
        colorS += smK * (prm.color - colorS);
        widthS += smK * (prm.width - widthS);
        // changement de couleur de bruit : fondu sortant/entrant de 2 × 15 ms, sans clic
        if (pendingType >= 0)
        {
            typeFade -= 2 * smK; if (typeFade <= 0) { typeFade = 0; curType = pendingType; pendingType = -1; g1.brown = g2.brown = 0; }
        }
        else if (typeFade < 1) typeFade = std::min (1.f, typeFade + 2 * smK);
        const float a = tilt (g1, g1.next (curType, brownLeak)), b = tilt (g2, g2.next (curType, brownLeak));
        const float th = widthS * (float) (PI / 4), cs = std::cos (th), sn = std::sin (th);
        const float k = gain * typeFade * typeFade * (3 - 2 * typeFade);
        float l = (cs * a + sn * b) * k, r = (cs * a - sn * b) * k;
        if (numCh == 1) { l = a * k; }
        if (prm.keepInput) { io[0][i] += l; if (numCh > 1) io[1][i] += r; }
        else { io[0][i] = l; if (numCh > 1) io[1][i] = r; }
        for (int c = 2; c < numCh; ++c) io[c][i] = 0;
        accL += (double) l * l; accR += (double) r * r;
        scopeAcc = std::max (scopeAcc, std::abs (l)); if (++scopeN >= (int) (sr / 400)) { meter.scope[(size_t) scopePos].store (scopeAcc); scopePos = (scopePos + 1) % 64; scopeAcc = 0; scopeN = 0; }
    }
    if (n > 0) { meter.rmsL.store ((float) std::sqrt (accL / n)); meter.rmsR.store ((float) std::sqrt (accR / n)); }
}
} // namespace rl::nz
