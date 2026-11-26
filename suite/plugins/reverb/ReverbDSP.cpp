#include "ReverbDSP.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace rl::rv
{
namespace { constexpr double PI = 3.14159265358979323846; }

// longueurs de base (ms, nombres « premiers entre eux » une fois en échantillons) par type
static const float kBase[3][8] = {
    { 11.3f, 13.7f, 16.1f, 18.9f, 21.4f, 24.2f, 27.3f, 30.1f },      // Room : petit, dense
    { 29.7f, 37.1f, 41.9f, 47.3f, 53.9f, 61.1f, 67.7f, 73.9f },      // Hall : grand, lent
    { 7.9f, 11.1f, 14.3f, 17.7f, 20.9f, 24.1f, 27.7f, 31.3f },       // Plate : très diffus, brillant
};
static const float kAp[3][4] = { { 4.1f, 3.3f, 2.3f, 1.7f }, { 6.7f, 5.1f, 3.7f, 2.9f }, { 3.1f, 2.6f, 1.9f, 1.3f } };

void Tank::prepare (double s)
{
    sr = s; maxLen = (int) (sr * 0.16) + 8;
    for (auto& l : line) l.assign ((size_t) maxLen, 0.f);
    for (auto& a : ap) a.assign ((size_t) (sr * 0.02) + 4, 0.f);
    erBuf.assign ((size_t) (sr * 0.1) + 4, 0.f);
    configure (1, 0.6f, s);
}

void Tank::configure (int type, float size, double s)
{
    sr = s; type = std::clamp (type, 0, 2);
    const float k = 0.45f + 1.1f * std::clamp (size, 0.f, 1.f);
    auto prime = [] (int n) { auto isP = [] (int x) { if (x < 2) return false; for (int d = 2; d * d <= x; ++d) if (x % d == 0) return false; return true; }; while (! isP (n)) ++n; return n; };
    for (int i = 0; i < N; ++i) { len[i] = std::clamp (prime ((int) (kBase[type][i] * k * sr / 1000.0)), 16, maxLen - 4); pos[i] %= len[i]; }
    for (int i = 0; i < AP; ++i) { apLen[i] = std::clamp (prime ((int) (kAp[type][i] * sr / 1000.0)), 8, (int) ap[i].size() - 2); apPos[i] %= apLen[i]; }
    apGain = type == 2 ? 0.72f : 0.6f;
    erAmt = type == 0 ? 0.35f : type == 1 ? 0.22f : 0.f;
    const float erMs[ER] = { 4.3f, 7.9f, 11.3f, 16.7f, 21.1f, 27.9f, 33.7f, 41.3f };
    for (int i = 0; i < ER; ++i) { erTap[i] = std::min ((int) erBuf.size() - 2, (int) (erMs[i] * k * sr / 1000.0)); erGain[i] = (i & 1 ? -1.f : 1.f) * std::pow (0.82f, (float) i); }
}

void Tank::clear()
{
    for (auto& l : line) std::fill (l.begin(), l.end(), 0.f);
    for (auto& a : ap) std::fill (a.begin(), a.end(), 0.f);
    std::fill (erBuf.begin(), erBuf.end(), 0.f); std::fill (std::begin (lp), std::end (lp), 0.f);
}

void Tank::process (float inL, float inR, float& outL, float& outR, float decay, float dampA, float width)
{
    // diffusion d'entrée (mono) : 4 passe-tout en série
    float x = 0.5f * (inL + inR);
    for (int i = 0; i < AP; ++i)
    {
        auto& b = ap[i]; const float d = b[(size_t) apPos[i]];
        const float y = -apGain * x + d; b[(size_t) apPos[i]] = x + apGain * y; x = y;
        if (++apPos[i] >= apLen[i]) apPos[i] = 0;
    }
    // premières réflexions
    erBuf[(size_t) erPos] = 0.5f * (inL + inR);
    float er = 0; const int es = (int) erBuf.size();
    for (int i = 0; i < ER; ++i) { int r = erPos - erTap[i]; if (r < 0) r += es; er += erBuf[(size_t) r] * erGain[i]; }
    if (++erPos >= es) erPos = 0;
    // FDN : sorties, mélange de Householder, amortissement, réinjection
    modPh += 0.7 / sr; if (modPh >= 1) modPh -= 1;
    float o[N], sum = 0;
    for (int i = 0; i < N; ++i)
    {
        // légère modulation de 2 lignes (évite le son métallique), lecture interpolée
        float rd = (float) pos[i];
        if (i < 2) rd -= 1.5f + 1.5f * (float) std::sin (2 * PI * (modPh + 0.5 * i));
        int r0 = (int) std::floor (rd); const float fr = rd - (float) r0; r0 %= len[i]; if (r0 < 0) r0 += len[i];
        const int r1 = (r0 + 1) % len[i];
        o[i] = line[i][(size_t) r0] * (1 - fr) + line[i][(size_t) r1] * fr;
        sum += o[i];
    }
    sum *= 2.f / N;
    for (int i = 0; i < N; ++i)
    {
        const float g = (float) std::pow (10.0, -3.0 * len[i] / (decay * sr));
        float y = (o[i] - sum) * g;
        lp[i] += dampA * (y - lp[i]); y = lp[i];
        line[i][(size_t) pos[i]] = y + x * (i & 1 ? -0.5f : 0.5f);
        if (++pos[i] >= len[i]) pos[i] = 0;
    }
    const float a = o[0] + o[2] + o[4] + o[6], b = o[1] + o[3] + o[5] + o[7];
    const float m = 0.5f * (a + b), sd = 0.5f * (a - b) * width;
    outL = (m + sd) * 0.35f + er * erAmt; outR = (m - sd) * 0.35f + er * erAmt * (1 - 0.3f * width);
}

void Engine::prepare (double s)
{
    sr = s; smK = (float) (1 - std::exp (-1.0 / (0.02 * sr)));
    tank[0].prepare (sr); tank[1].prepare (sr);
    pre.assign ((size_t) (sr * 0.26) * 2 + 4, 0.f);
    reset();
}

void Engine::reset()
{
    tank[0].clear(); tank[1].clear(); std::fill (pre.begin(), pre.end(), 0.f); prePos = 0;
    curType = prm.type; curSize = prm.size; tank[active].configure (curType, curSize, sr); xf = 1; pending = false;
    decayS = prm.decay; dampS = prm.dampHz; mixS = prm.mix; widthS = prm.width; lowS = prm.lowCutHz; predS = prm.predelayMs; inGate = prm.bypass ? 0.f : 1.f;
    hpZL = hpZR = hpXL = hpXR = 0;
}

void Engine::process (float* const* io, int numCh, int n)
{
    // type ou taille changés : on prépare l'autre moteur (vidé) et on fond de l'un à l'autre en 80 ms
    if ((prm.type != curType || std::abs (prm.size - curSize) > 0.004f) && ! pending && xf >= 1)
    {
        const int other = 1 - active; tank[other].clear(); tank[other].configure (prm.type, prm.size, sr);
        curType = prm.type; curSize = prm.size; pending = true; xf = 0;
    }
    const float xfInc = (float) (1.0 / (0.08 * sr));
    const int ps = (int) pre.size() / 2;
    double aIn = 0, aWet = 0;
    for (int i = 0; i < n; ++i)
    {
        decayS += smK * (prm.decay - decayS); dampS += smK * (prm.dampHz - dampS); mixS += smK * (prm.mix - mixS);
        widthS += smK * (prm.width - widthS); lowS += smK * (prm.lowCutHz - lowS); predS += smK * (prm.predelayMs - predS);
        inGate += smK * ((prm.bypass ? 0.f : 1.f) - inGate);
        const float dl = io[0][i], dr = numCh > 1 ? io[1][i] : dl;
        // pré-délai (stéréo entrelacé), lecture interpolée pour un réglage sans clic
        pre[(size_t) (2 * prePos)] = dl * inGate; pre[(size_t) (2 * prePos + 1)] = dr * inGate;
        const float pd = std::clamp (predS * 0.001f * (float) sr, 0.f, (float) ps - 2);
        float rp = (float) prePos - pd; if (rp < 0) rp += (float) ps;
        const int r0 = (int) rp, r1 = (r0 + 1) % ps; const float fr = rp - (float) r0;
        const float il = pre[(size_t) (2 * r0)] * (1 - fr) + pre[(size_t) (2 * r1)] * fr, ir = pre[(size_t) (2 * r0 + 1)] * (1 - fr) + pre[(size_t) (2 * r1 + 1)] * fr;
        if (++prePos >= ps) prePos = 0;
        const float dampA = (float) (1 - std::exp (-2 * PI * std::clamp (dampS, 500.f, 20000.f) / sr));
        const float dec = std::clamp (decayS, 0.1f, 30.f);
        float wl, wr; tank[active].process (il, ir, wl, wr, dec, dampA, widthS);
        if (pending)
        {
            float nl, nr; tank[1 - active].process (il, ir, nl, nr, dec, dampA, widthS);
            xf = std::min (1.f, xf + xfInc); const float g1 = std::sin (xf * 1.5707963f), g0 = std::cos (xf * 1.5707963f);
            wl = wl * g0 + nl * g1; wr = wr * g0 + nr * g1;
            if (xf >= 1) { active = 1 - active; pending = false; }
        }
        // coupe-bas (1er ordre) sur l'effet
        const float hpA = (float) std::exp (-2 * PI * lowS / sr);
        const float yl = hpA * (hpZL + wl - hpXL); hpXL = wl; hpZL = yl;
        const float yr = hpA * (hpZR + wr - hpXR); hpXR = wr; hpZR = yr;
        // mélange : en bypass, le son direct passe entier et la queue s'ajoute jusqu'à s'éteindre
        const float dry = prm.bypass ? 1.f : std::cos (mixS * 1.5707963f), wet = std::sin (mixS * 1.5707963f) * (prm.bypass ? 1.f : 1.f);
        float ol = dl * dry + yl * wet, orr = dr * dry + yr * wet;
        if (! std::isfinite (ol) || ! std::isfinite (orr)) { ol = dl; orr = dr; tank[0].clear(); tank[1].clear(); }
        io[0][i] = ol; if (numCh > 1) io[1][i] = orr;
        aIn += (double) dl * dl; aWet += (double) yl * yl;
    }
    if (n > 0)
    {
        meter.inL.store ((float) std::sqrt (aIn / n)); meter.wetL.store ((float) std::sqrt (aWet / n));
        accIn += (float) aIn; accWet += (float) aWet; accN += n;
        if (accN >= (int) (sr / 30)) { meter.decayEnv[(size_t) envPos].store (std::sqrt (accWet / accN)); envPos = (envPos + 1) % 48; accIn = accWet = 0; accN = 0; }
    }
}
} // namespace rl::rv
