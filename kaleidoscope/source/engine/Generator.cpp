#include "Generator.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <sstream>

namespace kal
{
int scalePitch (const ScaleDef& s, int root, int degree, int baseOctave)
{
    const int n = s.n; int oct = degree >= 0 ? degree / n : -((-degree + n - 1) / n);
    const int idx = degree - oct * n;
    return 12 * (baseOctave + oct) + root + s.iv[idx];
}
bool inScale (const ScaleDef& s, int root, int pitch)
{
    const int pc = ((pitch - root) % 12 + 12) % 12;
    for (int i = 0; i < s.n; ++i) if (s.iv[i] == pc) return true;
    return false;
}
static int degreeOf (const ScaleDef& s, int root, int pitch)       // note de la gamme -> degré absolu (octave 5 = 0..n-1)
{
    const int rel = pitch - (60 + root); const int oct = rel >= 0 ? rel / 12 : -((-rel + 11) / 12);
    const int pc = rel - oct * 12; int best = 0, bd = 99;
    for (int i = 0; i < s.n; ++i) { const int d = std::abs (s.iv[i] - pc); if (d < bd) { bd = d; best = i; } }
    return oct * s.n + best;
}

// ---------------------------------------------------------------------------
int Generator::pickScale (const Request& r, Rng& rng) const
{
    if (r.scale >= 0) return clampI (r.scale, 0, data::kNumScales - 1);
    const auto& st = style (r.style); const auto& md = data::kMoods[clampI (r.mood, 0, data::kNumMoods - 1)];
    // gammes du style, favorisées si l'humeur les aime aussi
    double w[6] {}; double tot = 0; int n = 0;
    for (int i = 0; i < 6 && st.scales[i] >= 0; ++i, ++n)
    {
        w[i] = (i == 0 ? 1.6 : 1.0);
        for (int k = 0; k < 5; ++k) if (md.scales[k] == st.scales[i]) w[i] *= 2.4;
        tot += w[i];
    }
    double x = rng.uni() * tot;
    for (int i = 0; i < n; ++i) { if ((x -= w[i]) <= 0) return st.scales[i]; }
    return st.scales[0];
}

// ---------------------------------------------------------------------------
// Solo : motif rythmique, phrase en questions-réponses, contour, notes de l'accord sur les appuis
void Generator::solo (Pattern& p, Rng& rng, const Request& r, int sc) const
{
    const auto& st = style (r.style); const auto& md = data::kMoods[clampI (r.mood, 0, data::kNumMoods - 1)];
    const auto& S = scaleDef (sc); const float* W = data::kRhythms[st.rhythm];
    const double dens = std::clamp (st.density * md.density * (1.0 + 0.6 * r.density), 0.12, 1.0);
    const double leapP = std::clamp (st.leap + (rng.uni() - 0.5) * 0.15, 0.05, 0.7);
    const double legato = std::clamp (0.5 * st.legato + 0.5 * md.legato, 0.1, 1.0);
    const int center = degreeOf (S, r.root, 60 + r.root + st.reg + md.reg + 12 * r.octave + (rng.range (-2, 2)));
    const int span = S.n + rng.range (0, S.n / 2);           // amplitude (en degrés)
    const int nBars = p.bars;

    // 1) motif rythmique d'une mesure (positions en double-croches, ou triolets)
    auto makeRhythm = [&] (bool trip) {
        std::vector<int> on;
        if (trip)
        {   // triolets de croches : 12 positions par mesure
            for (int i = 0; i < 12; ++i) { const double w = (i % 3 == 0 ? 0.9 : 0.45) * (i % 6 == 0 ? 1.1 : 1.0); if (rng.chance (w * dens * 0.95)) on.push_back (i * 4); }
        }
        else for (int i = 0; i < 16; ++i) { const double w = W[i]; if (rng.chance (std::min (1.0, w * (dens * 1.35 + 0.05)))) on.push_back (i * 3); }
        if (on.size() < 2) { on.clear(); on.push_back (0); on.push_back (rng.chance (0.5) ? 18 : 24); }
        // silences voulus
        if (st.rest > 0.3 && on.size() > 3 && rng.chance (st.rest)) on.erase (on.begin() + (long) rng.range (1, (int) on.size() - 1));
        return on;
    };
    auto mutate = [&] (std::vector<int> on, double amt) {
        for (auto& o : on) if (o > 0 && rng.chance (amt * 0.35)) { o += (rng.chance (0.5) ? 3 : -3); o = std::clamp (o, 3, TPBAR - 3); }
        if (rng.chance (amt * 0.5) && on.size() > 2) on.erase (on.begin() + (long) rng.range (1, (int) on.size() - 1));
        if (rng.chance (amt * 0.5)) { const int pos = rng.range (1, 15) * 3; on.push_back (pos); }
        std::sort (on.begin(), on.end()); on.erase (std::unique (on.begin(), on.end()), on.end());
        return on;
    };
    const bool tripFeel = rng.chance (st.triplet);
    const auto motif = makeRhythm (tripFeel);
    // forme de la phrase : A A' B A'' (ou A B A C), selon la répétition du style
    std::vector<std::vector<int>> bars (nBars);
    for (int b = 0; b < nBars; ++b)
    {
        const int inPhrase = b % 4;
        if (inPhrase == 0) bars[b] = b == 0 ? motif : mutate (motif, 0.4);
        else if (inPhrase == 2) bars[b] = rng.chance (st.repeat) ? mutate (motif, 0.6) : makeRhythm (tripFeel && rng.chance (0.6));
        else bars[b] = rng.chance (st.repeat) ? mutate (motif, 0.3) : makeRhythm (tripFeel && rng.chance (0.6));
    }
    // cadence : la dernière mesure finit par une note plus longue
    if (! bars.back().empty() && bars.back().size() > 2 && rng.chance (0.75)) { auto& lb = bars.back(); while (lb.size() > 1 && lb.back() > 30) lb.pop_back(); }

    // 2) hauteurs : marche sur les degrés, attirée par les notes de l'accord aux appuis, contour en arche
    int deg = center + rng.range (-1, 1);
    const int contour = rng.range (0, 3);   // 0 arche, 1 descendant, 2 montant, 3 vague
    std::vector<int> motifDeg; int prevRoot = 0;
    for (int b = 0; b < nBars; ++b)
    {
        const int ci = std::min ((int) p.chordRoots.size() - 1, b * p.chordsPerBar);
        const int croot = p.chordRoots.empty() ? 0 : p.chordRoots[(size_t) ci];
        const bool seq = b % 4 != 0 && ! motifDeg.empty() && rng.chance (st.repeat * 0.55) && bars[b].size() == motifDeg.size();
        std::vector<int> barDeg;
        for (size_t k = 0; k < bars[b].size(); ++k)
        {
            const int pos = bars[b][k];
            const bool strong = pos % 12 == 0;
            const int cr = p.chordsPerBar == 2 && pos >= 24 && ci + 1 < (int) p.chordRoots.size() ? p.chordRoots[(size_t) ci + 1] : croot;
            int d;
            if (seq) d = motifDeg[k] + (croot - prevRoot);                 // séquence : le motif suit l'accord
            else
            {
                const double t = (b * TPBAR + pos) / p.lengthTicks;
                double target = center;
                if (contour == 0) target += span * 0.45 * std::sin (t * 3.14159);
                else if (contour == 1) target += span * 0.4 * (0.5 - t);
                else if (contour == 2) target += span * 0.4 * (t - 0.5);
                else target += span * 0.3 * std::sin (t * 6.283 * 2);
                int step;
                if (rng.chance (0.12)) step = 0;
                else if (rng.chance (leapP)) step = rng.range (2, 4) * (rng.chance (0.5) ? 1 : -1);
                else step = rng.chance (0.5) ? 1 : -1;
                if (deg + step > target + 2) step = -std::abs (step);
                if (deg + step < target - 2) step = std::abs (step);
                d = deg + step;
            }
            // appuis : note de l'accord (fondamentale, tierce, quinte, + extensions)
            if (strong && rng.chance (0.8 - 0.25 * md.tension))
            {
                int best = d, bd = 99;
                for (int o = -2; o <= 2; ++o) for (int ct : { 0, 2, 4 })
                { const int c = cr + ct + o * S.n; if (std::abs (c - d) < bd) { bd = std::abs (c - d); best = c; } }
                d = best;
            }
            d = std::clamp (d, center - span, center + span);
            barDeg.push_back (d); deg = d;
        }
        if (b % 4 == 0) { motifDeg = barDeg; prevRoot = croot; }
        // dernière note de la phrase : stable (fondamentale ou quinte de l'accord)
        if ((b % 4 == 3 || b == nBars - 1) && ! barDeg.empty()) { int& last = barDeg.back(); const int c = rng.chance (0.6) ? croot : croot + 2; int o = (int) std::round ((last - c) / (double) S.n); last = c + o * S.n; }
        // notes
        for (size_t k = 0; k < bars[b].size(); ++k)
        {
            const double start = b * TPBAR + bars[b][k];
            const double next = k + 1 < bars[b].size() ? b * TPBAR + bars[b][k + 1] : (b + 1) * TPBAR + (b + 1 < nBars && ! bars[b + 1].empty() ? bars[b + 1][0] : 0);
            double len = std::max (1.5, (next - start) * (0.35 + 0.6 * legato));
            if (k + 1 == bars[b].size() && (b == nBars - 1 || b % 4 == 3)) len = std::max (len, std::min (24.0, (b + 1) * TPBAR - start));
            const float acc = (bars[b][k] % 12 == 0 ? 0.92f : (bars[b][k] % 6 == 0 ? 0.8f : 0.7f));
            const float vel = std::clamp (acc * (0.85f + 0.3f * (float) rng.uni()) * (0.8f + 0.3f * md.energy), 0.3f, 1.f);
            Note n { start, len, scalePitch (S, r.root, barDeg[k]), vel };
            // ornement : petite note avant un appui
            if (st.orn > 0 && bars[b][k] % 12 == 0 && k > 0 && rng.chance (st.orn * 0.5))
            {
                const int g = scalePitch (S, r.root, barDeg[k] + (rng.chance (0.5) ? 1 : -1));
                if (start >= 1.5) p.notes.push_back ({ start - 1.5, 1.4, g, vel * 0.7f });
            }
            p.notes.push_back (n);
        }
    }
    // swing sur les double-croches impaires
    if (st.swing > 0.01) for (auto& n : p.notes) { const int pos = (int) std::round (n.start); if (pos % 6 == 3) n.start += st.swing * 1.6; }
    // monophonique : aucune note ne chevauche la suivante
    std::sort (p.notes.begin(), p.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    for (size_t i = 0; i + 1 < p.notes.size(); )
    {
        if (p.notes[i + 1].start - p.notes[i].start < 0.5) { p.notes.erase (p.notes.begin() + (long) i + 1); continue; }
        p.notes[i].len = std::min (p.notes[i].len, p.notes[i + 1].start - p.notes[i].start - 0.05); ++i;
    }
}

// ---------------------------------------------------------------------------
// Accompagnement : voicings enchaînés au plus près, motif rythmique selon le style
void Generator::accomp (Pattern& p, Rng& rng, const Request& r, int sc) const
{
    const auto& st = style (r.style); const auto& md = data::kMoods[clampI (r.mood, 0, data::kNumMoods - 1)];
    const auto& S = scaleDef (sc); const float* W = data::kRhythms[st.rhythm];
    const double ext = std::clamp (0.5 * st.ext + 0.5 * md.tension + (rng.uni() - 0.5) * 0.2, 0.0, 1.0);
    const double dens = std::clamp (st.density * md.density * (1.0 + 0.5 * r.density), 0.15, 1.0);
    int type = st.accomp;
    if (rng.chance (0.18)) type = rng.chance (0.5) ? Pad : Broken;          // un peu de variété dans le style
    const int base = 60 + r.root + 12 * r.octave + (st.reg < -1 ? -5 : 0);
    std::vector<int> prev;
    const int nCh = (int) p.chordRoots.size(), chLen = TPBAR / p.chordsPerBar;
    for (int c = 0; c < nCh; ++c)
    {
        const int cr = p.chordRoots[(size_t) c];
        // degrés empilés en tierces (dans la gamme : tout reste dans la tonalité)
        std::vector<int> degs { cr, cr + 2, cr + 4 };
        if (S.n >= 7 && rng.chance (ext)) degs.push_back (cr + 6);
        if (S.n >= 7 && rng.chance (ext * 0.7)) degs.push_back (cr + 8);
        if (S.n < 7 && rng.chance (ext)) degs.push_back (cr + 3);
        std::vector<int> pitches; for (int d : degs) pitches.push_back (scalePitch (S, r.root, d, 4));
        // renversement le plus proche du voicing précédent (conduite des voix)
        auto centerOf = [] (const std::vector<int>& v) { double s = 0; for (int x : v) s += x; return s / std::max<size_t> (1, v.size()); };
        const double target = prev.empty() ? base : centerOf (prev);
        std::vector<int> best; double bd = 1e9;
        for (int inv = 0; inv < (int) pitches.size(); ++inv)
            for (int o = -1; o <= 1; ++o)
            {
                std::vector<int> v = pitches;
                for (int i = 0; i < inv; ++i) v[(size_t) i] += 12;
                for (auto& x : v) x += 12 * o;
                std::sort (v.begin(), v.end());
                const double d = std::abs (centerOf (v) - target) + (v.back() - v.front() > 16 ? 6 : 0) + (v.front() < 50 ? 8 : 0) + (v.back() > 84 ? 8 : 0);
                if (d < bd) { bd = d; best = v; }
            }
        prev = best;
        const int bassP = scalePitch (S, r.root, cr, 3) + 12 * r.octave;
        const double t0 = c * chLen;
        const float v0 = 0.62f + 0.25f * md.energy;
        auto chord = [&] (double at, double len, float vel, double strum = 0) {
            for (size_t i = 0; i < best.size(); ++i) p.notes.push_back ({ at + strum * (double) i, std::max (1.0, len - strum * (double) i), best[i], std::clamp (vel * (0.9f + 0.15f * (float) rng.uni()), 0.25f, 1.f) });
        };
        const bool withBass = type == Pad || type == Broken || type == Comp || type == Picking || type == Rumba || type == Strum || type == Log;
        if (withBass) p.notes.push_back ({ t0, chLen * 0.95, bassP, v0 });
        switch (type)
        {
            case Pad: chord (t0, chLen * 0.98, v0 * 0.9f); break;
            case Stabs: case Log:
                for (int i = 0; i < 16 * chLen / TPBAR; ++i) if (W[i] > 0.65 || (W[i] > 0.4 && rng.chance (dens))) chord (t0 + i * 3, type == Log ? 6 : 2.2, v0 * (W[i] > 0.8 ? 1.f : 0.85f));
                break;
            case Offbeat: for (int i = 2; i < 16 * chLen / TPBAR; i += 4) chord (t0 + i * 3, 2.4, v0); break;
            case Pump: case Pulse8: for (int i = 0; i < 8 * chLen / TPBAR; ++i) chord (t0 + i * 6, 4.5, v0 * (i % 2 ? 0.75f : 1.f)); break;
            case Strum: for (int i = 0; i < 4 * chLen / TPBAR; ++i) chord (t0 + i * 12, 11, v0 * (i % 2 ? 0.8f : 1.f), 0.35); break;
            case Comp:
                for (int i = 0; i < 16 * chLen / TPBAR; ++i) if (W[i] > 0.6 && rng.chance (0.7 * dens + 0.2)) chord (t0 + i * 3, 4 + rng.range (0, 6), v0 * 0.9f);
                break;
            case Arp: case Broken: case Picking: case Rumba: default:
            {
                chord (t0, chLen * 0.5, v0 * 0.5f);          // le voicing posé, puis le motif dessus
                std::vector<int> seq = best;
                if (type == Broken) seq = { best.front(), best.size() > 2 ? best[2] : best.back(), best.back(), best.size() > 1 ? best[1] : best.front() };
                const int step = type == Arp ? 3 : (type == Broken ? 6 : 3);
                int k = rng.range (0, (int) seq.size() - 1), dir = 1;
                for (int pos = 0; pos < chLen; pos += step)
                {
                    const int i16 = (pos / 3) % 16;
                    if ((type == Picking || type == Rumba) && W[i16] < 0.35 && ! rng.chance (dens * 0.6)) continue;
                    const int pitch = seq[(size_t) k] + (type == Picking && i16 % 4 == 2 ? 12 : 0);
                    p.notes.push_back ({ t0 + pos, step * 0.9, pitch, v0 * (i16 % 4 == 0 ? 1.f : 0.8f) });
                    k += dir; if (k >= (int) seq.size() || k < 0) { if (type == Arp && rng.chance (0.5)) { dir = -dir; k += 2 * dir; } else k = 0; }
                    k = std::clamp (k, 0, (int) seq.size() - 1);
                }
                break;
            }
        }
    }
    if (st.swing > 0.01) for (auto& n : p.notes) { const int pos = (int) std::round (n.start); if (pos % 6 == 3) n.start += st.swing * 1.6; }
    std::sort (p.notes.begin(), p.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
}

// ---------------------------------------------------------------------------
Pattern Generator::candidate (const Request& r, uint64_t seed) const
{
    Rng rng (seed);
    Pattern p; p.seed = seed; p.style = r.style; p.mood = r.mood; p.root = ((r.root % 12) + 12) % 12; p.mode = r.mode;
    p.bars = std::clamp (r.bars, 1, 8); p.lengthTicks = p.bars * TPBAR;
    const auto& st = style (r.style);
    p.scale = pickScale (r, rng);
    // progression : une du style, éventuellement modifiée (substitutions)
    const auto& S = scaleDef (p.scale);
    const int* pr = st.progs[rng.range (0, std::max (0, st.nProgs - 1))];
    p.chordsPerBar = (st.cpb == 2 && p.bars <= 4 && rng.chance (0.6)) ? 2 : 1;
    const int nCh = p.bars * p.chordsPerBar;
    for (int c = 0; c < nCh; ++c)
    {
        int d = pr[c % 4] - 1;
        if (rng.chance (0.12) && c % 4 != 0) d = (d + (rng.chance (0.5) ? 2 : 5)) % 7;   // substitution (relatif, quinte)
        p.chordRoots.push_back (((d % S.n) + S.n) % S.n);
    }
    if (r.mode == Mode::Solo) solo (p, rng, r, p.scale); else accomp (p, rng, r, p.scale);
    fingerprint (p);
    p.feat = measure (p);
    return p;
}

void Generator::fingerprint (Pattern& p)
{
    std::set<uint32_t> s;
    auto h = [] (uint32_t a, uint32_t b, uint32_t c, uint32_t d) { uint32_t x = a * 73856093u ^ b * 19349663u ^ c * 83492791u ^ d * 2654435761u; return x; };
    // départs groupés (accords) -> une ligne par instant
    std::vector<std::pair<double, int>> line;
    for (auto& n : p.notes) { if (! line.empty() && std::abs (line.back().first - n.start) < 0.5) { line.back().second = std::max (line.back().second, n.pitch); continue; } line.push_back ({ n.start, n.pitch }); }
    for (size_t i = 0; i + 3 < line.size(); ++i)
    {
        const int a = line[i + 1].second - line[i].second, b = line[i + 2].second - line[i + 1].second, c = line[i + 3].second - line[i + 2].second;
        s.insert (h (1, (uint32_t) (a + 64), (uint32_t) (b + 64), (uint32_t) (c + 64)));
        const int r1 = (int) std::round (line[i + 1].first - line[i].first), r2 = (int) std::round (line[i + 2].first - line[i + 1].first), r3 = (int) std::round (line[i + 3].first - line[i + 2].first);
        s.insert (h (2, (uint32_t) r1, (uint32_t) r2, (uint32_t) r3));
        s.insert (h (3, (uint32_t) (((int) std::round (line[i].first)) % TPBAR), (uint32_t) (a + 64), (uint32_t) r1));
    }
    for (size_t i = 0; i < p.chordRoots.size(); ++i) s.insert (h (4, (uint32_t) i, (uint32_t) p.chordRoots[i], (uint32_t) p.scale));
    p.print.assign (s.begin(), s.end());
}

double Generator::similarity (const Pattern& a, const Pattern& b)
{
    if (a.print.empty() || b.print.empty()) return 0;
    size_t i = 0, j = 0, inter = 0;
    while (i < a.print.size() && j < b.print.size()) { if (a.print[i] == b.print[j]) { ++inter; ++i; ++j; } else if (a.print[i] < b.print[j]) ++i; else ++j; }
    return (double) inter / (double) (a.print.size() + b.print.size() - inter);
}

Features Generator::measure (const Pattern& p)
{
    Features f; if (p.notes.empty()) return f;
    const double beats = p.lengthTicks / TPB;
    std::vector<const Note*> line;
    for (auto& n : p.notes) { if (! line.empty() && std::abs (line.back()->start - n.start) < 0.5) { if (n.pitch > line.back()->pitch) line.back() = &n; continue; } line.push_back (&n); }
    f.density = (float) std::min (1.0, line.size() / beats / 4.0);
    double iv = 0, leaps = 0, up = 0; int lo = 127, hi = 0; double mean = 0, covered = 0;
    for (size_t i = 0; i < line.size(); ++i)
    {
        lo = std::min (lo, line[i]->pitch); hi = std::max (hi, line[i]->pitch); mean += line[i]->pitch; covered += line[i]->len;
        if (i) { const int d = line[i]->pitch - line[i - 1]->pitch; iv += std::abs (d); leaps += std::abs (d) > 4; up += d > 0; }
    }
    const double n = (double) line.size();
    f.meanInterval = (float) std::min (1.0, iv / std::max (1.0, n - 1) / 7.0);
    f.leap = (float) (leaps / std::max (1.0, n - 1));
    f.up = (float) (up / std::max (1.0, n - 1));
    f.reg = (float) std::clamp ((mean / n - 48.0) / 36.0, 0.0, 1.0);
    f.range = (float) std::min (1.0, (hi - lo) / 24.0);
    f.rest = (float) std::clamp (1.0 - covered / p.lengthTicks, 0.0, 1.0);
    int off = 0, trip = 0; for (auto* x : line) { const int pos = ((int) std::round (x->start)) % 12; off += pos != 0 && pos != 6; trip += pos % 4 == 0 && pos % 3 != 0; }
    f.sync = (float) (off / n); f.triplet = (float) (trip / n);
    // tension : notes hors de l'accord sur les appuis + accords enrichis
    if (! p.chordRoots.empty())
    {
        const auto& S = scaleDef (p.scale); int strong = 0, outside = 0;
        for (auto* x : line)
        {
            if (((int) std::round (x->start)) % 12 != 0) continue;
            ++strong;
            const int ci = std::min ((int) p.chordRoots.size() - 1, (int) (x->start / (TPBAR / p.chordsPerBar)));
            const int cr = p.chordRoots[(size_t) ci], d = degreeOf (S, p.root, x->pitch);
            const int rel = ((d - cr) % S.n + S.n) % S.n; if (rel != 0 && rel != 2 && rel != 4) ++outside;
        }
        f.tension = (float) (outside / std::max (1.0, (double) strong));
    }
    return f;
}

double Generator::coherence (const Pattern& p) const
{
    if (p.notes.empty()) return -10;
    const auto& f = p.feat; const auto& st = style (p.style);
    double s = 1.0;
    s -= std::abs (f.density - st.density * 0.55) * 0.8;                  // densité proche du style
    s -= f.tension * (p.mode == Mode::Solo ? 0.6 : 0.2);                  // appuis sur l'accord
    s -= std::max (0.0, f.range - 0.85) * 2;                              // pas trop étendu
    s -= std::max (0.0, 0.15 - f.range) * 2 * (p.mode == Mode::Solo);     // ni trop plat
    s -= std::max (0.0, f.leap - 0.5);
    int same = 0, worst = 0; for (size_t i = 1; i < p.notes.size(); ++i) { same = p.notes[i].pitch == p.notes[i - 1].pitch ? same + 1 : 0; worst = std::max (worst, same); }
    s -= std::max (0, worst - 3) * 0.25;                                  // pas de note répétée sans fin
    return s;
}

Pattern Generator::generate (const Request& r, const Profile* prof)
{
    Pattern best; double bestScore = -1e9;
    const int nCand = r.seed ? 1 : std::max (1, candidates);
    for (int k = 0; k < nCand; ++k)
    {
        const uint64_t seed = r.seed ? r.seed : (counter = counter * 6364136223846793005ull + 1442695040888963407ull) ^ (uint64_t) k * 0x9E37ull;
        Pattern c = candidate (r, seed);
        double maxSim = 0; for (auto& h : history) { Pattern tmp; tmp.print = h; maxSim = std::max (maxSim, similarity (c, tmp)); }
        if (maxSim > 0.6 && ! r.seed) continue;                            // trop proche d'une proposition récente : rejetée
        const double coh = coherence (c), nov = 1.0 - maxSim;
        double score = coh * 1.0 + nov * 0.6;
        if (prof && r.personal > 0) score += r.personal * prof->score (c.feat);
        if (score > bestScore) { bestScore = score; best = std::move (c); lastNovelty = nov; lastCoherence = coh; }
    }
    if (! best.valid()) best = candidate (r, counter ^ 0xABCDEFull);
    return best;
}

Pattern Generator::decline (const Pattern& ref, const Request& rq, const Profile* prof)
{
    if (! ref.valid()) return generate (rq, prof);
    const double s = std::clamp ((double) rq.variation, 0.0, 1.0);
    const double lo = 0.2 + 0.35 * (1 - s), hi = 0.85 - 0.25 * s, mid = 0.5 * (lo + hi);
    Pattern best; double bestScore = -1e9;
    const auto& S = scaleDef (ref.scale);
    for (int k = 0; k < std::max (1, candidates); ++k)
    {
        Rng rng ((counter = counter * 6364136223846793005ull + 1442695040888963407ull) ^ ref.seed);
        Pattern p = ref; p.seed = rng.next();
        // 1) quelques accords remplacés (relatif, quinte)
        for (size_t c = 1; c < p.chordRoots.size(); ++c) if (rng.chance (s * 0.25)) p.chordRoots[c] = (p.chordRoots[c] + (rng.chance (0.5) ? 2 : 5)) % S.n;
        // 2) une mesure (pas la première) recomposée dans le même style
        if (rng.chance (s * 0.5) && p.bars > 1)
        {
            Request r2 = rq; r2.style = ref.style; r2.mood = ref.mood; r2.root = ref.root; r2.scale = ref.scale; r2.mode = ref.mode; r2.bars = ref.bars;
            Pattern fresh = candidate (r2, rng.next());
            const int bar = rng.range (1, p.bars - 1);
            p.notes.erase (std::remove_if (p.notes.begin(), p.notes.end(), [&] (const Note& n) { return (int) (n.start / TPBAR) == bar; }), p.notes.end());
            for (auto& n : fresh.notes) if ((int) (n.start / TPBAR) == bar) p.notes.push_back (n);
        }
        // 3) notes déplacées d'un degré, rythmes décalés, ornements
        for (auto& n : p.notes)
        {
            const bool strong = ((int) std::round (n.start)) % 12 == 0;
            if (! strong && rng.chance (s * 0.45)) { const int d = degreeOf (S, ref.root, n.pitch) + (rng.chance (0.5) ? 1 : -1); n.pitch = scalePitch (S, ref.root, d); }
            if (! strong && rng.chance (s * 0.2)) n.start = std::clamp (n.start + (rng.chance (0.5) ? 3 : -3), 0.0, p.lengthTicks - 1.5);
        }
        if (ref.mode == Mode::Solo)
        {
            std::sort (p.notes.begin(), p.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
            std::vector<Note> mono; for (auto& n : p.notes) if (mono.empty() || n.start - mono.back().start > 0.6) mono.push_back (n);
            for (size_t i = 0; i + 1 < mono.size(); ++i) mono[i].len = std::min (mono[i].len, mono[i + 1].start - mono[i].start - 0.05);
            p.notes = mono;
        }
        else std::sort (p.notes.begin(), p.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
        fingerprint (p); p.feat = measure (p);
        const double sim = similarity (p, ref);
        double maxHist = 0; for (auto& h : history) { Pattern tmp; tmp.print = h; maxHist = std::max (maxHist, similarity (p, tmp)); }
        double score = -std::abs (sim - mid) * 3 - (sim < lo || sim > hi ? 1.0 : 0) + coherence (p) * 0.5 + (1 - maxHist) * 0.2;
        if (prof && rq.personal > 0) score += rq.personal * prof->score (p.feat) * 0.5;
        if (score > bestScore) { bestScore = score; best = std::move (p); lastNovelty = 1 - maxHist; }
    }
    return best;
}

void Generator::remember (const Pattern& p) { history.push_back (p.print); if (history.size() > 64) history.erase (history.begin()); }

// ---------------------------------------------------------------------------
void Profile::like (const Features& f, double w) { for (int i = 0; i < Features::N; ++i) pos[(size_t) i] += f[i] * w; nPos += (int) std::round (w); if (nPos <= 0) nPos = 1; }
void Profile::dislike (const Features& f) { for (int i = 0; i < Features::N; ++i) neg[(size_t) i] += f[i]; ++nNeg; }
double Profile::score (const Features& f) const
{
    double s = 0;
    // distance moyenne aux goûts (traits entre 0 et 1) ; la confiance monte avec le nombre de retours
    // densité, registre et syncope comptent le plus dans ce qu'on aime d'une mélodie
    static const double w[Features::N] = { 3.0, 1.0, 1.0, 1.5, 1.5, 1.0, 1.0, 1.0, 1.0, 0.5 };
    if (nPos > 0) { double d = 0; for (int i = 0; i < Features::N; ++i) d += w[i] * std::abs (f[i] - pos[(size_t) i] / nPos); s -= d / Features::N * 12.0 * std::min (1.0, nPos / 5.0); }
    if (nNeg > 0) { double d = 0; for (int i = 0; i < Features::N; ++i) d += w[i] * std::abs (f[i] - neg[(size_t) i] / nNeg); s += d / Features::N * 6.0 * std::min (1.0, nNeg / 5.0); }
    return s;
}
std::string Profile::toString() const
{
    std::ostringstream o; o << nPos << ' ' << nNeg; for (auto v : pos) o << ' ' << v; for (auto v : neg) o << ' ' << v; return o.str();
}
bool Profile::fromString (const std::string& s)
{
    std::istringstream in (s); Profile p; if (! (in >> p.nPos >> p.nNeg)) return false;
    for (auto& v : p.pos) { if (! (in >> v)) return false; }
    for (auto& v : p.neg) { if (! (in >> v)) return false; }
    *this = p; return true;
}
} // namespace kal
