#pragma once
// =============================================================================
// Kaleidoscope — générateur de mélodies et d'accompagnements (C++ pur, testable sans JUCE).
// Grammaires par style (tools/styles.py) + humeur ; Solo (monophonique) ou Accompagnement (voicings) ;
// Keys (tonalité + gamme imposées) ou Drop (tonalité estimée puis corrigée par l'utilisateur) ;
// Déclinaison (variation liée à une référence) ; anti-répétition par empreintes ; mode Personal.
// Chaque proposition a une graine : la même graine et les mêmes réglages redonnent la même proposition.
// =============================================================================
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace kal
{
struct ScaleDef { const char* id; int n; int iv[8]; };
struct MoodDef { const char* id; int scales[5]; float density; int reg; float tension, legato, energy, tempo; };
struct StyleDef
{
    const char* id; int family, tmin, tmax; int scales[6]; int nProgs; int progs[8][4]; int rhythm, accomp;
    float density, swing, triplet; int reg; float leap, rest, legato, ext, repeat, orn; int cpb; float energy; int palette;
};
namespace data
{
#include "../gen/Styles.inc"
}

constexpr int TPB = 12;          // pas par temps (double-croche = 3, triolet de croche = 4)
constexpr int TPBAR = 48;        // 4/4

struct Note { double start = 0, len = 3; int pitch = 60; float vel = 0.8f; };
enum class Mode { Solo = 0, Accomp = 1 };
enum Accomp { Pad = 0, Stabs, Offbeat, Arp, Broken, Comp, Pump, Picking, Pulse8, Rumba, Strum, Log };

// traits musicaux : empreinte, mode Personal et pilotage du kaléidoscope
struct Features
{
    float density = 0, meanInterval = 0, leap = 0, reg = 0, sync = 0, rest = 0, range = 0, tension = 0, triplet = 0, up = 0;
    static constexpr int N = 10;
    float operator[] (int i) const { const float* p = &density; return p[i]; }
    float& operator[] (int i) { float* p = &density; return p[i]; }
};

struct Pattern
{
    std::vector<Note> notes;
    int bars = 4; double lengthTicks = 4 * TPBAR;
    uint64_t seed = 0;
    int style = 0, mood = 0, root = 0, scale = 0; Mode mode = Mode::Solo;
    std::vector<int> chordRoots;          // degré (0..n-1) de chaque accord
    int chordsPerBar = 1;
    Features feat; std::vector<uint32_t> print;
    bool valid() const { return ! notes.empty(); }
};

struct Request
{
    int style = 0, mood = 0;
    Mode mode = Mode::Solo;
    int root = 0;            // 0 = do
    int scale = -1;          // -1 = selon le style et l'humeur
    int bars = 4;
    float density = 0;       // -1..1 autour de la densité du style
    int octave = 0;          // -2..2
    float variation = 0.5f;  // force de la déclinaison
    float personal = 0.5f;   // poids du profil Personal
    uint64_t seed = 0;       // 0 = nouvelle graine
};

// profil Personal : moyennes des traits aimés / gardés / à refaire
struct Profile
{
    std::array<double, Features::N> pos {}, neg {};
    int nPos = 0, nNeg = 0;
    void like (const Features& f, double w = 1.0);
    void dislike (const Features& f);
    void reset() { *this = Profile {}; }
    double score (const Features& f) const;        // > 0 = proche des goûts
    std::string toString() const;
    bool fromString (const std::string&);
};

class Generator
{
public:
    static int numStyles() { return data::kNumStyles; }
    static int numMoods() { return data::kNumMoods; }
    static int numScales() { return data::kNumScales; }
    static const StyleDef& style (int i) { return data::kStyles[clampI (i, 0, data::kNumStyles - 1)]; }
    static const ScaleDef& scaleDef (int i) { return data::kScales[clampI (i, 0, data::kNumScales - 1)]; }

    Pattern generate (const Request& r, const Profile* profile = nullptr);         // Générer
    Pattern decline (const Pattern& ref, const Request& r, const Profile* profile = nullptr);   // Déclinaison
    void remember (const Pattern& p);              // historique anti-répétition
    void clearHistory() { history.clear(); }
    static double similarity (const Pattern& a, const Pattern& b);
    static Features measure (const Pattern& p);
    int candidates = 24;
    double lastNovelty = 1, lastCoherence = 0;

private:
    struct Rng { uint64_t s; explicit Rng (uint64_t x) : s (x ? x : 0x9E3779B97F4A7C15ull) {}
        uint64_t next() { s += 0x9E3779B97F4A7C15ull; uint64_t z = s; z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull; z = (z ^ (z >> 27)) * 0x94D049BB133111EBull; return z ^ (z >> 31); }
        double uni() { return (double) (next() >> 11) * (1.0 / 9007199254740992.0); }
        int range (int a, int b) { return a + (int) (uni() * (b - a + 1)); }
        bool chance (double p) { return uni() < p; } };
    static int clampI (int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
    Pattern candidate (const Request& r, uint64_t seed) const;
    void solo (Pattern& p, Rng& rng, const Request& r, int scale) const;
    void accomp (Pattern& p, Rng& rng, const Request& r, int scale) const;
    double coherence (const Pattern& p) const;
    static void fingerprint (Pattern& p);
    int pickScale (const Request& r, Rng& rng) const;
    std::vector<std::vector<uint32_t>> history;
    uint64_t counter = 0x51EDC0DEull;
};

// utilitaires partagés
int scalePitch (const ScaleDef& s, int root, int degree, int baseOctave = 5);   // degré (peut être négatif ou > n) -> note MIDI
bool inScale (const ScaleDef& s, int root, int pitch);
} // namespace kal
