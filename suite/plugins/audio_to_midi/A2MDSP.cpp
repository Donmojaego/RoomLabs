#include "A2MDSP.h"
#include <algorithm>
#include <cmath>

namespace rl::a2m
{
void Engine::prepare (double s)
{
    sr = s; dec = sr > 64000 ? 4 : 2;
    ring.assign (W, 0.f); frame.assign (W, 0.f); diff.assign (W / 2, 0.f);
    reset();
}

void Engine::reset()
{
    std::fill (ring.begin(), ring.end(), 0.f); rpos = hopCnt = decCnt = 0; decAcc = aa1 = aa2 = 0;
    curNote = candNote = -1; candFrames = offFrames = 0; prevDb = -120;
    lpS = hpS = hpX = envL = envM = envH = slowL = slowM = slowH = 0; refractory = drumOff = pend = 0; pL = pM = pH = 0; drumNote = -1;
    meter.note.store (-1);
}

void Engine::analyseFrame (int off, Event* ev, int& ne)
{
    for (int i = 0; i < W; ++i) frame[(size_t) i] = ring[(size_t) ((rpos + i) % W)];
    double e = 0; for (float v : frame) e += (double) v * v;
    const float db = (float) (10 * std::log10 (e / W + 1e-12));
    meter.levelDb.store (db);
    // YIN : différence cumulée normalisée
    const int maxT = W / 2;
    const double dsr = sr / dec;
    const int minT = std::max (2, (int) (dsr / 1500.0));
    diff[0] = 1;
    double run = 0; int best = -1;
    for (int t = 1; t < maxT; ++t)
    {
        double d = 0;
        for (int i = 0; i < maxT; ++i) { const double x = frame[(size_t) i] - frame[(size_t) (i + t)]; d += x * x; }
        run += d; diff[(size_t) t] = (float) (run > 0 ? d * t / run : 1);
    }
    for (int t = minT; t < maxT - 1; ++t)
        if (diff[(size_t) t] < 0.15f) { while (t + 1 < maxT && diff[(size_t) (t + 1)] < diff[(size_t) t]) ++t; best = t; break; }
    float hz = 0, conf = 0;
    if (best > 0)
    {
        const float a = diff[(size_t) (best - 1)], b = diff[(size_t) best], c = diff[(size_t) (best + 1)];
        const float den = a - 2 * b + c, sh = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
        hz = (float) (dsr / (best + std::clamp (sh, -1.f, 1.f))); conf = 1 - b;
    }
    meter.hz.store (hz); meter.conf.store (conf);
    const int minFrames = std::max (1, (int) std::lround (prm.minLenMs * 0.001 * dsr / Hop));
    const bool voiced = db > prm.threshDb && hz > 0;
    int note = -1;
    if (voiced) { note = (int) std::lround (69 + 12 * std::log2 (hz / 440.0)) + prm.transpose; if (note < prm.lowNote || note > prm.highNote) note = -1; }
    const bool attack = db > prevDb + 9 && db > prm.threshDb;
    prevDb = db;
    const int vel = (int) std::clamp (100 + prm.velSens * (db + 18) * 3.f, 1.f, 127.f);
    if (note >= 0 && (note != curNote || attack))
    {
        if (note == candNote) ++candFrames; else { candNote = note; candFrames = 1; }
        if (candFrames >= minFrames || (attack && candFrames >= 1 && curNote == note))
        {
            if (curNote >= 0) emit (ev, ne, off, curNote, 0);
            curNote = note; emit (ev, ne, off, note, vel); candFrames = 0; candNote = -1;
        }
        offFrames = 0;
    }
    else if (note < 0)
    {
        candNote = -1; candFrames = 0;
        if (curNote >= 0 && ++offFrames >= 2) { emit (ev, ne, off, curNote, 0); curNote = -1; offFrames = 0; }
    }
    else offFrames = 0;
    meter.note.store (curNote);
}

int Engine::process (const float* const* in, int numCh, int n, Event* ev)
{
    int ne = 0;
    if (prm.mode == 0)
    {
        if (drumNote >= 0) { emit (ev, ne, 0, drumNote, 0); drumNote = -1; }
        const float aa = 0.42f;   // passe-bas 2 pôles simple avant décimation
        for (int i = 0; i < n; ++i)
        {
            float x = in[0][i]; if (numCh > 1) x = 0.5f * (x + in[1][i]);
            if (! std::isfinite (x)) x = 0;
            aa1 += aa * (x - aa1); aa2 += aa * (aa1 - aa2);
            if (++decCnt >= dec)
            {
                decCnt = 0; ring[(size_t) rpos] = aa2; rpos = (rpos + 1) % W;
                if (++hopCnt >= Hop) { hopCnt = 0; analyseFrame (i, ev, ne); }
            }
        }
    }
    else
    {
        if (curNote >= 0) { emit (ev, ne, 0, curNote, 0); curNote = -1; }
        const float aL = (float) (1 - std::exp (-2 * 3.14159265 * 150 / sr)), aH = (float) std::exp (-2 * 3.14159265 * 5000 / sr);
        const float fast = (float) (1 - std::exp (-1.0 / (0.002 * sr))), slow = (float) (1 - std::exp (-1.0 / (0.15 * sr))), rel = (float) (1 - std::exp (-1.0 / (0.03 * sr)));
        const float ratio = std::pow (10.f, (std::clamp (-prm.threshDb, 10.f, 70.f) / 10.f) * 0.1f + 0.25f);   // sensibilité
        const float floorA = std::pow (10.f, prm.threshDb / 20.f);
        for (int i = 0; i < n; ++i)
        {
            float x = in[0][i]; if (numCh > 1) x = 0.5f * (x + in[1][i]);
            if (! std::isfinite (x)) x = 0;
            lpS += aL * (x - lpS); const float h = aH * (hpS + x - hpX); hpX = x; hpS = h; const float m = x - lpS - h;
            auto env = [&] (float& e, float v) { const float a = std::abs (v); e += (a > e ? fast : rel) * (a - e); };
            env (envL, lpS); env (envM, m); env (envH, h);
            slowL += slow * (envL - slowL); slowM += slow * (envM - slowM); slowH += slow * (envH - slowH);
            if (drumNote >= 0 && --drumOff <= 0) { emit (ev, ne, i, drumNote, 0); drumNote = -1; }
            if (pend > 0)
            {   // fenêtre de classement de 6 ms après l'attaque
                pL = std::max (pL, envL); pM = std::max (pM, envM); pH = std::max (pH, envH);
                if (--pend == 0)
                {
                    const int note = (pL * 1.3f >= pM && pL * 1.3f >= pH) ? 36 : (pH > pM ? 42 : 38);
                    if (drumNote >= 0) emit (ev, ne, i, drumNote, 0);
                    const float pk = 20 * std::log10 (pL + pM + pH + 1e-9f);
                    const int vel = (int) std::clamp (100 + prm.velSens * (pk + 12) * 3.f, 1.f, 127.f);
                    drumNote = std::clamp (note + prm.transpose, 0, 127); emit (ev, ne, i, drumNote, vel);
                    drumOff = (int) (0.06 * sr); meter.note.store (drumNote);
                }
                continue;
            }
            if (refractory > 0) { --refractory; continue; }
            const float rL = envL / (slowL + 1e-5f), rM = envM / (slowM + 1e-5f), rH = envH / (slowH + 1e-5f);
            const float tot = envL + envM + envH;
            if (tot > floorA && std::max ({ rL, rM, rH }) > ratio)
            {
                pend = std::max (1, (int) (0.006 * sr)); pL = pM = pH = 0;
                refractory = (int) (std::max (30.f, prm.minLenMs) * 0.001 * sr);
            }
        }
        meter.levelDb.store (20 * std::log10 (envL + envM + envH + 1e-9f));
    }
    return ne;
}
} // namespace rl::a2m
