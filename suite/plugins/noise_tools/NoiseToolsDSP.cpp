#include "NoiseToolsDSP.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace rl::nt
{
namespace { constexpr double PI = 3.14159265358979323846; }

void Engine::prepare (double sampleRate, int channels)
{
    sr = sampleRate > 1000 ? sampleRate : 48000;
    ch.assign ((size_t) std::max (1, channels), Chan {});
    for (auto& c : ch)
    {
        c.inFifo.assign (HOP, 0.f); c.ctx.assign (3 * HOP, 0.f); c.rep.assign (3 * HOP, 0);
        c.stIn.assign (N, 0.f); c.stOut.assign (N, 0.f);
        c.prevP.assign (BINS, 0.f); c.noiseP.assign (BINS, 0.f); c.prevG.assign (BINS, 1.f); c.gSmooth.assign (BINS, 1.f); c.learnAcc.assign (BINS, 0.f);
        c.spec.assign (N, {});
        c.outFifo.assign (8 * N, 0.f);
        c.dry.assign ((size_t) latency() + 8, 0.f);
    }
    win.resize (N); for (int i = 0; i < N; ++i) win[(size_t) i] = (float) std::sqrt (0.5 - 0.5 * std::cos (2 * PI * i / N));
    tmp.assign (3 * HOP, 0.f); tmp2.assign (3 * HOP, 0.f); cums.assign (3 * HOP + 1, 0.f);
    lsL.assign ((size_t) MAXGAP * (AR + 1), 0.0); lsV.assign ((size_t) MAXGAP + 2 * AR, 0.0); lsU.assign ((size_t) MAXGAP, 0);
    tw.resize (N / 2); for (int i = 0; i < N / 2; ++i) tw[(size_t) i] = std::polar (1.f, (float) (-2 * PI * i / N));
    bitrev.resize (N); for (int i = 0, j = 0; i < N; ++i) { bitrev[(size_t) i] = j; int bit = N >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; }
    bandIn.assign (32, 0.f); bandOut.assign (32, 0.f); bandNoise.assign (32, 0.f);
    humMode = -1;
    reset();
}

void Engine::reset()
{
    for (auto& c : ch)
    {
        std::fill (c.inFifo.begin(), c.inFifo.end(), 0.f); std::fill (c.ctx.begin(), c.ctx.end(), 0.f); std::fill (c.rep.begin(), c.rep.end(), (char) 0);
        std::fill (c.stIn.begin(), c.stIn.end(), 0.f); std::fill (c.stOut.begin(), c.stOut.end(), 0.f);
        std::fill (c.prevP.begin(), c.prevP.end(), 0.f); std::fill (c.noiseP.begin(), c.noiseP.end(), 0.f);
        std::fill (c.prevG.begin(), c.prevG.end(), 1.f); std::fill (c.gSmooth.begin(), c.gSmooth.end(), 1.f);
        std::fill (c.learnAcc.begin(), c.learnAcc.end(), 0.f); c.learnN = 0;
        std::fill (c.outFifo.begin(), c.outFifo.end(), 0.f);
        std::fill (c.dry.begin(), c.dry.end(), 0.f);
        c.inPos = 0; c.stPos = 0; c.dryPos = 0; c.outRead = 0; c.outWrite = HOP;    // HOP de silence d'avance : jamais de manque
        for (auto& h : c.hum) h.fill (0.0);
        c.rmsIn = c.rmsOut = 0;
    }
    xfBypass = prm.bypass ? 1.f : 0.f; xfCompare = prm.compare ? 1.f : 0.f; matchGain = 1;
    totalClicks = totalClipRuns = 0;
}

void Engine::fft (std::complex<float>* a, bool inv)
{
    for (int i = 0; i < N; ++i) if (i < bitrev[(size_t) i]) std::swap (a[i], a[bitrev[(size_t) i]]);
    for (int len = 2; len <= N; len <<= 1)
    {
        const int step = N / len;
        for (int i = 0; i < N; i += len)
            for (int j = 0; j < len / 2; ++j)
            {
                auto w = tw[(size_t) (j * step)]; if (inv) w = std::conj (w);
                const auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v; a[i + j + len / 2] = u - v;
            }
    }
}

// ---- coefficients AR (autocorrélation fenêtrée + Levinson) ; false si le signal est trop faible
bool Engine::arCoefs (const float* x, int len, double* a)
{
    double r[AR + 1] = {};
    for (int i = 0; i < len; ++i) tmp[(size_t) i] = x[i] * (float) (0.5 - 0.5 * std::cos (2 * PI * (i + .5) / len));
    for (int k = 0; k <= AR; ++k) { double s = 0; for (int i = k; i < len; ++i) s += (double) tmp[(size_t) i] * tmp[(size_t) (i - k)]; r[k] = s; }
    if (r[0] < 1e-12) return false;
    r[0] *= 1.0 + 1e-6;
    for (int j = 0; j <= AR; ++j) a[j] = j == 0 ? 1 : 0;
    double e = r[0];
    for (int i = 1; i <= AR; ++i)
    {
        double acc = r[i]; for (int j = 1; j < i; ++j) acc += a[j] * r[i - j];
        const double kref = -acc / e; double na[AR + 1]; for (int j = 0; j <= AR; ++j) na[j] = a[j];
        for (int j = 1; j < i; ++j) na[j] = a[j] + kref * a[i - j];
        na[i] = kref; for (int j = 0; j <= AR; ++j) a[j] = na[j];
        e *= (1 - kref * kref); if (e <= 0) return false;
    }
    return true;
}

// ---- De-clip : plateaux écrêtés reconstruits par un polynôme cubique (moindres carrés sur 6 + 6 points)
void Engine::declip (float* x, char* rep, int len, int a0, int b0)
{
    if ((int) cums.size() < len + 1) cums.resize ((size_t) len + 1);
    // crête du signal d'origine : les échantillons déjà reconstruits (qui dépassent le plateau) sont ignorés
    float peak = 0; for (int i = 0; i < len; ++i) if (! rep[i]) peak = std::max (peak, std::abs (x[i]));
    if (peak < 1e-4f) return;
    // un vrai écrêtage numérique est PLAT : tolérance de 0,01 % à 0,2 % de la crête selon la sensibilité
    const float tol = 1e-4f * std::pow (10.f, prm.dpSens * 1.3f), thr = peak * (1.f - tol);
    const int minRun = prm.dpSens > .6f ? 2 : 3;
    // 1) repérage des plateaux (tableau fixe : pas d'allocation)
    constexpr int MAXR = 128; int ri[MAXR], rj[MAXR]; float rs[MAXR]; int nr = 0;
    for (int i = a0; i < b0 && nr < MAXR; ++i)
    {
        if (rep[i] || std::abs (x[i]) < thr) continue;
        const float sgn = x[i] > 0 ? 1.f : -1.f;
        int j = i; while (j < len && ! rep[j] && x[j] * sgn >= thr) ++j;
        const int runLen = j - i;
        if (runLen >= minRun && runLen <= MAXGAP && i >= AR && j + AR <= len && (rep[i - 1] || x[i - 1] * sgn < thr)) { ri[nr] = i; rj[nr] = j; rs[nr] = sgn; ++nr; }
        i = j;
    }
    if (nr == 0) return;
    // 2) interpolation AR par moindres carrés (Janssen) : les échantillons écrêtés sont les inconnues qui minimisent
    //    l'erreur de prédiction ; contrainte « au-delà du plateau » imposée en fixant les échantillons fautifs et en résolvant à nouveau.
    //    Deux passes : la 2e réestime le modèle AR sur le signal déjà réparé.
    double ar[AR + 1];
    for (int pass = 0; pass < 2; ++pass)
    {
        if (! arCoefs (x, len, ar)) return;
        double R[AR + 1];
        for (int d = 0; d <= AR; ++d) { double t = 0; for (int q = 0; q + d <= AR; ++q) t += ar[q] * ar[q + d]; R[d] = t; }
        for (int r = 0; r < nr; ++r)
        {
            const int i = ri[r], j = rj[r]; const float sgn = rs[r];
            for (int k = i; k < j; ++k) x[k] = sgn * thr;
            char* fixedS = lsU.data(); for (int k = 0; k < j - i; ++k) fixedS[k] = 0;
            for (int it = 0; it < 4; ++it)
            {
                // inconnues = échantillons non fixés du plateau
                int idx[MAXGAP]; int n = 0; for (int k = i; k < j; ++k) if (! fixedS[k - i]) idx[n++] = k;
                if (n == 0) break;
                for (int u = 0; u < n; ++u) x[idx[u]] = 0;
                // résidu avec inconnues à zéro sur t ∈ [i, j-1+AR]
                double* e = lsV.data();
                for (int t = i; t < j + AR; ++t) { double acc = 0; for (int q = 0; q <= AR; ++q) acc += ar[q] * x[t - q]; e[t - i] = acc; }
                // système (bande, symétrique défini positif) : M[u][v] = R(|k_u - k_v|), b[u] = -Σ_q a_q e(k_u + q)
                double* L = lsL.data(); double bvec[MAXGAP];
                for (int u = 0; u < n; ++u) { double acc = 0; for (int q = 0; q <= AR; ++q) acc += ar[q] * e[idx[u] + q - i]; bvec[u] = -acc; }
                // Cholesky (matrice dense limitée à la bande AR via les distances d'indices)
                const int W = AR;   // les indices non consécutifs restent dans une bande ≤ AR en position
                bool ok = true;
                for (int u = 0; u < n && ok; ++u)
                    for (int v = std::max (0, u - W); v <= u; ++v)
                    {
                        const int dd = idx[u] - idx[v]; double sum = dd <= AR ? R[dd] : 0.0;
                        if (u == v) sum += 1e-9 * R[0];
                        for (int k = std::max (0, u - W); k < v; ++k) if (v - k <= W) sum -= L[(size_t) u * (W + 1) + (u - k)] * L[(size_t) v * (W + 1) + (v - k)];
                        if (u == v) { if (sum <= 0) { ok = false; break; } L[(size_t) u * (W + 1)] = std::sqrt (sum); }
                        else L[(size_t) u * (W + 1) + (u - v)] = sum / L[(size_t) v * (W + 1)];
                    }
                if (! ok) { for (int u = 0; u < n; ++u) x[idx[u]] = sgn * thr; break; }
                for (int u = 0; u < n; ++u) { double sum = bvec[u]; for (int k = std::max (0, u - W); k < u; ++k) sum -= L[(size_t) u * (W + 1) + (u - k)] * bvec[k]; bvec[u] = sum / L[(size_t) u * (W + 1)]; }
                for (int u = n - 1; u >= 0; --u) { double sum = bvec[u]; for (int k = u + 1; k <= std::min (n - 1, u + W); ++k) sum -= L[(size_t) k * (W + 1) + (k - u)] * bvec[k]; bvec[u] = sum / L[(size_t) u * (W + 1)]; }
                bool viol = false;
                for (int u = 0; u < n; ++u)
                {
                    double y = bvec[u] * sgn;
                    if (y < thr) { y = thr; fixedS[idx[u] - i] = 1; viol = true; }
                    else if (y > thr * 2.5) { y = thr * 2.5; fixedS[idx[u] - i] = 1; viol = true; }
                    x[idx[u]] = (float) (sgn * y);
                }
                if (! viol) break;
            }
        }
    }
    for (int r = 0; r < nr; ++r) for (int k = ri[r]; k < rj[r]; ++k) rep[k] = 1;
    clipCount += nr; totalClipRuns += nr;
}

// ---- De-click : résidu de prédiction linéaire (AR 16), seuil robuste (MAD), interpolation AR avant/arrière
int Engine::declick (float* x, int len, int a0, int b0)
{
    double a[AR + 1];
    if (! arCoefs (x, len, a)) return 0;
    // résidu et seuil robuste
    for (int n = AR; n < len; ++n) { double s = x[n]; for (int k = 1; k <= AR; ++k) s += a[k] * x[n - k]; tmp2[(size_t) n] = (float) s; }
    for (int n = 0; n < AR; ++n) tmp2[(size_t) n] = 0;
    for (int n = 0; n < len; ++n) tmp[(size_t) n] = std::abs (tmp2[(size_t) n]);
    std::nth_element (tmp.begin() + AR, tmp.begin() + (AR + len) / 2, tmp.begin() + len);
    const float mad = std::max (1e-7f, tmp[(size_t) ((AR + len) / 2)] / 0.6745f);
    float frameRms = 0; for (int i2 = 0; i2 < len; ++i2) frameRms += x[i2] * x[i2]; frameRms = std::sqrt (frameRms / len);
    const float thr = std::max ({ mad * (20.f - 12.f * prm.dcSens), 3e-4f, frameRms * 0.02f * (1.5f - prm.dcSens) });
    // moyenne locale du résidu (± 64 échantillons) : un clic est une pointe isolée, une attaque musicale non
    std::vector<float>& cs = cums; cs.resize ((size_t) len + 1); cs[0] = 0;
    for (int i2 = 0; i2 < len; ++i2) cs[(size_t) i2 + 1] = cs[(size_t) i2] + std::abs (tmp2[(size_t) i2]);
    auto localMean = [&] (int q) { const int lo = std::max (0, q - 64), hi = std::min (len, q + 65); return (cs[(size_t) hi] - cs[(size_t) lo]) / (float) (hi - lo); };
    const float kLocal = 12.f - 6.f * prm.dcSens;
    const int maxLen = std::max (2, (int) (prm.dcLenMs * 0.001 * sr));
    int found = 0;
    for (int n = std::max (a0, AR + 1); n < b0; ++n)
    {
        if (std::abs (tmp2[(size_t) n]) <= thr || std::abs (tmp2[(size_t) n]) <= kLocal * localMean (n)) continue;
        int m = n, last = n;
        while (m < len - AR - 2 && m - last <= 4) { if (std::abs (tmp2[(size_t) m]) > thr) last = m; ++m; }
        const int s0 = std::max (AR + 1, n - 2), s1 = std::min (len - AR - 1, last + 3);     // [s0, s1)
        const int L = s1 - s0;
        if (L > maxLen) { n = last + 1; continue; }     // trop long : transitoire musical, on ne touche pas
        // prédiction avant
        for (int i = s0; i < s1; ++i) { double s = 0; for (int k = 1; k <= AR; ++k) s -= a[k] * (i - k >= s0 ? tmp[(size_t) (i - k)] : x[i - k]); tmp[(size_t) i] = (float) s; }
        // prédiction arrière (mêmes coefficients, signal retourné)
        for (int i = s1 - 1; i >= s0; --i) { double s = 0; for (int k = 1; k <= AR; ++k) s -= a[k] * (i + k < s1 ? tmp2[(size_t) (i + k)] : x[i + k]); tmp2[(size_t) i] = (float) s; }
        for (int i = s0; i < s1; ++i) { const float w = (float) (i - s0 + 1) / (float) (L + 1); x[i] = tmp[(size_t) i] * (1 - w) + tmp2[(size_t) i] * w; }
        ++found;
        // le résidu au-delà de la réparation n'est plus valable : on le recalcule localement
        for (int q = s1; q < std::min (len, s1 + AR); ++q) { double s = x[q]; for (int k = 1; k <= AR; ++k) s += a[k] * x[q - k]; tmp2[(size_t) q] = (float) s; }
        n = s1;
    }
    return found;
}

void Engine::humFilter (Chan& c, float* x, int n)
{
    const int mode = prm.hum;
    if (mode != humMode)
    {
        humMode = mode;
        const double f0 = mode == 2 ? 60.0 : 50.0;
        for (int k = 0; k < 8; ++k)
        {
            const double f = f0 * (k + 1), w = 2 * PI * f / sr, Q = 60.0 + 12 * k, al = std::sin (w) / (2 * Q), cw = std::cos (w), a0 = 1 + al;
            humCoef[(size_t) k] = { 1 / a0, -2 * cw / a0, 1 / a0, -2 * cw / a0, (1 - al) / a0 };
        }
        for (auto& cc : ch) for (auto& h : cc.hum) h.fill (0.0);
    }
    if (mode == 0) return;
    const int nh = std::min (8, (int) (sr * 0.45 / (mode == 2 ? 60.0 : 50.0)));
    for (int i = 0; i < n; ++i)
    {
        double y = x[i];
        for (int k = 0; k < nh; ++k)
        {
            auto& s = c.hum[(size_t) k]; const auto& cf = humCoef[(size_t) k];
            const double o = cf[0] * y + cf[1] * s[0] + cf[2] * s[1] - cf[3] * s[2] - cf[4] * s[3];
            s[1] = s[0]; s[0] = y; s[3] = s[2]; s[2] = o; y = o;
        }
        x[i] = (float) y;
    }
}

void Engine::frameTime (Chan& c)
{
    // contexte : [précédent | courant | suivant]
    std::memmove (c.ctx.data(), c.ctx.data() + HOP, sizeof (float) * 2 * HOP);
    std::memmove (c.rep.data(), c.rep.data() + HOP, (size_t) 2 * HOP); std::fill (c.rep.begin() + 2 * HOP, c.rep.end(), (char) 0);
    std::copy (c.inFifo.begin(), c.inFifo.end(), c.ctx.begin() + 2 * HOP);
    // le bloc « courant » a ses deux voisins : on répare dans le contexte
    if (prm.dpOn) declip (c.ctx.data(), c.rep.data(), 3 * HOP, HOP, 2 * HOP);
    if (prm.dcOn) { const int f = declick (c.ctx.data(), 3 * HOP, HOP, 2 * HOP); clickCount += f; totalClicks += f; }
    float cur[HOP]; std::copy (c.ctx.begin() + HOP, c.ctx.begin() + 2 * HOP, cur);
    double e0 = 0; for (float v : cur) e0 += v * v;
    humFilter (c, cur, HOP);
    if (prm.hum) { double e1 = 0; for (float v : cur) e1 += v * v; meter.humDb.store ((float) (10 * std::log10 ((e0 + 1e-12) / (e1 + 1e-12)))); }
    // vers la STFT
    std::memmove (c.stIn.data(), c.stIn.data() + HOP, sizeof (float) * (N - HOP));
    std::copy (cur, cur + HOP, c.stIn.begin() + (N - HOP));
    stftFrame (c);
}

void Engine::stftFrame (Chan& c)
{
    for (int i = 0; i < N; ++i) c.spec[(size_t) i] = { c.stIn[(size_t) i] * win[(size_t) i], 0.f };
    fft (c.spec.data(), false);
    const bool learning = prm.learn;
    const float floorG = std::pow (10.f, -prm.dnAmount / 20.f), over = 1.f + 3.f * prm.dnSens;
    const bool first = c.prevP[1] == 0 && c.noiseP[1] == 0;
    if (learning) c.learnN++;
    else if (c.learnN > 0) { for (int k = 0; k < BINS; ++k) c.noiseP[(size_t) k] = c.learnAcc[(size_t) k] / (float) c.learnN; c.learnN = 0; std::fill (c.learnAcc.begin(), c.learnAcc.end(), 0.f); }
    float* G = tmp.data();       // gains (BINS <= 3 * HOP)
    for (int k = 0; k < BINS; ++k)
    {
        const float P = std::norm (c.spec[(size_t) k]);
        float& nz = c.noiseP[(size_t) k];
        if (learning) { c.learnAcc[(size_t) k] += P; nz = c.learnAcc[(size_t) k] / (float) c.learnN; }
        else if (first) nz = P;
        else
        {
            const float Ps = 0.8f * c.prevP[(size_t) k] + 0.2f * P;      // puissance lissée
            if (Ps < 3.f * nz) nz += 0.04f * (Ps - nz);          // zone de bruit : on suit la moyenne
            else nz = nz * 1.003f + 1e-14f;                     // signal présent : remontée lente (≈ +1,2 dB/s)
        }
        float g = 1.f;
        if (prm.dnOn)
        {
            const float nn = over * nz + 1e-14f;
            const float post = P / nn, prio = 0.97f * c.prevG[(size_t) k] * c.prevG[(size_t) k] * c.prevP[(size_t) k] / nn + 0.03f * std::max (post - 1.f, 0.f);
            const float w = prio / (1.f + prio);
            g = std::max (floorG, w * std::sqrt (w));          // Wiener renforcé : moins de bruit résiduel entre les notes
        }
        c.prevG[(size_t) k] = g; c.prevP[(size_t) k] = P;
        G[k] = g;
    }
    // lissage en fréquence et dans le temps (bruit musical réduit)
    for (int k = 0; k < BINS; ++k)
    {
        const float gs = k > 0 && k < BINS - 1 ? 0.25f * G[k - 1] + 0.5f * G[k] + 0.25f * G[k + 1] : G[k];
        float& s = c.gSmooth[(size_t) k];
        s = gs > s ? 0.7f * gs + 0.3f * s : 0.45f * gs + 0.55f * s;
    }
    // mesures (32 bandes log 40 Hz - 20 kHz) et réduction moyenne pondérée
    double wIn = 0, wOut = 0;
    std::fill (bandIn.begin(), bandIn.end(), 0.f); std::fill (bandOut.begin(), bandOut.end(), 0.f); std::fill (bandNoise.begin(), bandNoise.end(), 0.f);
    for (int k = 1; k < BINS; ++k)
    {
        const double f = k * sr / N; if (f < 40) continue;
        const int b = std::clamp ((int) (std::log (f / 40.0) / std::log (500.0) * 32), 0, 31);
        const float P = c.prevP[(size_t) k], g = c.gSmooth[(size_t) k];
        bandIn[(size_t) b] += P; bandOut[(size_t) b] += P * g * g; bandNoise[(size_t) b] += c.noiseP[(size_t) k];
        wIn += P; wOut += P * g * g;
    }
    for (int k = 0; k < BINS; ++k) { const float g = c.gSmooth[(size_t) k]; c.spec[(size_t) k] *= g; if (k > 0 && k < N / 2) c.spec[(size_t) (N - k)] = std::conj (c.spec[(size_t) k]); }
    fft (c.spec.data(), true);
    // somme des fenêtres sqrt-Hann² avec recouvrement 75 % = 2 → 0,5 ; et 1/N de la FFT inverse
    const float sc = 0.5f / N;
    for (int i = 0; i < N; ++i) c.stOut[(size_t) i] += c.spec[(size_t) i].real() * win[(size_t) i] * sc;
    // HOP échantillons terminés vers la file de sortie
    const int sz = (int) c.outFifo.size();
    for (int i = 0; i < HOP; ++i) { c.outFifo[(size_t) c.outWrite] = c.stOut[(size_t) i]; c.outWrite = (c.outWrite + 1) % sz; }
    std::memmove (c.stOut.data(), c.stOut.data() + HOP, sizeof (float) * (N - HOP));
    std::fill (c.stOut.begin() + (N - HOP), c.stOut.end(), 0.f);
    if (&c == &ch[0])
    {
        meter.reductionDb.store (wIn > 1e-12 ? (float) (10 * std::log10 (wIn / std::max (1e-12, wOut))) : 0.f);
        for (int b = 0; b < 32; ++b)
        {
            meter.specIn[(size_t) b].store (bandIn[(size_t) b]); meter.specOut[(size_t) b].store (bandOut[(size_t) b]); meter.noise[(size_t) b].store (bandNoise[(size_t) b]);
        }
        if (++statFrames >= (int) (sr / HOP))      // ≈ chaque seconde
        {
            meter.clicksPerSec.store ((float) clickCount / ch.size()); meter.clipsPerSec.store ((float) clipCount / ch.size());
            clickCount = clipCount = 0; statFrames = 0;
        }
    }
}

void Engine::process (float* const* io, int numCh, int n)
{
    const int C = std::min (numCh, (int) ch.size());
    const float target = std::pow (10.f, prm.outDb / 20.f) * (prm.dpOn ? std::pow (10.f, prm.dpMarginDb / 20.f) : 1.f);
    const float xfK = (float) (1.0 - std::exp (-1.0 / (0.02 * sr))), rmsK = (float) (1.0 - std::exp (-1.0 / (0.4 * sr)));
    const int dsz = latency() + 8;
    float pkIn[2] = {}, pkOut[2] = {};
    for (int i = 0; i < n; ++i)
    {
        xfBypass += xfK * ((prm.bypass ? 1.f : 0.f) - xfBypass);
        xfCompare += xfK * ((prm.compare ? 1.f : 0.f) - xfCompare);
        outGain += xfK * (target - outGain);
        for (int cI = 0; cI < C; ++cI)
        {
            auto& c = ch[(size_t) cI];
            const float x = io[cI][i];
            pkIn[std::min (cI, 1)] = std::max (pkIn[std::min (cI, 1)], std::abs (x));
            // dry retardé de la latence exacte
            c.dry[(size_t) c.dryPos] = x;
            const int rd = (c.dryPos + 1) % dsz;    // tampon de taille latence + 8 : on lit « latence » échantillons en arrière
            const float d = c.dry[(size_t) ((c.dryPos - latency() + dsz) % dsz)];
            (void) rd;
            c.dryPos = (c.dryPos + 1) % dsz;
            // chaîne de traitement
            c.inFifo[(size_t) c.inPos++] = x;
            if (c.inPos == HOP) { c.inPos = 0; frameTime (c); }
            const int sz = (int) c.outFifo.size();
            const float w = c.outFifo[(size_t) c.outRead] * outGain; c.outRead = (c.outRead + 1) % sz;
            c.rmsIn += rmsK * (d * d - c.rmsIn); c.rmsOut += rmsK * (w * w - c.rmsOut);
            float y = w * (1 - xfBypass) + d * xfBypass;
            y = y * (1 - xfCompare) + d * matchGain * xfCompare;
            if (! std::isfinite (y)) y = 0;
            io[cI][i] = y;
            pkOut[std::min (cI, 1)] = std::max (pkOut[std::min (cI, 1)], std::abs (y));
        }
    }
    // A/B à niveau comparable : l'original est joué au niveau du traité
    double ri = 0, ro = 0; for (int cI = 0; cI < C; ++cI) { ri += ch[(size_t) cI].rmsIn; ro += ch[(size_t) cI].rmsOut; }
    const float want = ri > 1e-10 ? (float) std::sqrt (ro / ri) : 1.f;
    matchGain = std::clamp (matchGain + 0.2f * (want - matchGain), 0.05f, 4.f);
    meter.inL.store (pkIn[0]); meter.inR.store (C > 1 ? pkIn[1] : pkIn[0]);
    meter.outL.store (pkOut[0]); meter.outR.store (C > 1 ? pkOut[1] : pkOut[0]);
}
} // namespace rl::nt
