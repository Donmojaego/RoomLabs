#include "PluginProcessor.h"
#include "Params.h"
#include <BinaryData.h>
#include <set>
#if ! KAL_HEADLESS
 #include "PluginEditor.h"
#endif

namespace
{
// champ de kal::SynthParams correspondant à chaque paramètre de son
struct FieldRef { const char* id; float kal::SynthParams::* f; int kal::SynthParams::* i; bool kal::SynthParams::* b; };
#define F(x) { #x, &kal::SynthParams::x, nullptr, nullptr }
#define I(x) { #x, nullptr, &kal::SynthParams::x, nullptr }
#define B(x) { #x, nullptr, nullptr, &kal::SynthParams::x }
const FieldRef kFields[] = {
    B(wtOn), I(tableA), F(posA), I(tableB), F(posB), F(mixB), F(semiB), F(fineB), I(unison), F(detune), F(spread), F(sub), F(noise), F(wtEnv), F(wtLfo),
    I(fMode), B(f24), F(cutoff), F(reso), F(fEnv), F(keytrack), F(drive), F(aA), F(aD), F(aS), F(aR), F(fA), F(fD), F(fS), F(fR),
    F(lfoRate), F(lfoVib), F(lfoCut), F(lfoAmp), B(granOn), F(granSize), F(granDensity), F(granSpray), F(granPitch), F(granPos), F(granMix),
    B(smpOn), F(smpLevel), F(smpTune), B(mono), F(glideMs), F(bendRange), F(fxDrive), F(chorus), F(delay), F(delayFb), I(delayDiv),
    F(reverb), F(revSize), F(width), F(tilt), F(levelDb) };
#undef F
#undef I
#undef B
constexpr int kNumFields = (int) (sizeof (kFields) / sizeof (kFields[0]));

int indexOf (const juce::String& id, bool style)
{
    const int n = style ? kal::data::kNumStyles : kal::data::kNumMoods;
    for (int i = 0; i < n; ++i)
        if (id == (style ? kal::data::kStyles[i].id : kal::data::kMoods[i].id)) return i;
    return -1;
}

juce::String patternToString (const kal::Pattern& p)
{
    juce::StringArray head { juce::String ((juce::int64) p.seed), juce::String (p.style), juce::String (p.mood), juce::String (p.root), juce::String (p.scale),
                             juce::String (p.bars), juce::String ((int) p.mode), juce::String (p.chordsPerBar) };
    juce::StringArray chords; for (int c : p.chordRoots) chords.add (juce::String (c));
    juce::StringArray notes; for (auto& n : p.notes) notes.add (juce::String (n.start, 3) + "," + juce::String (n.len, 3) + "," + juce::String (n.pitch) + "," + juce::String (n.vel, 3));
    return head.joinIntoString (",") + "|" + chords.joinIntoString (",") + "|" + notes.joinIntoString (";");
}

bool patternFromString (const juce::String& s, kal::Pattern& p)
{
    auto parts = juce::StringArray::fromTokens (s, "|", ""); if (parts.size() < 3) return false;
    auto h = juce::StringArray::fromTokens (parts[0], ",", ""); if (h.size() < 8) return false;
    p = kal::Pattern {};
    p.seed = (uint64_t) h[0].getLargeIntValue(); p.style = h[1].getIntValue(); p.mood = h[2].getIntValue(); p.root = h[3].getIntValue(); p.scale = h[4].getIntValue();
    p.bars = juce::jlimit (1, 16, h[5].getIntValue()); p.mode = (kal::Mode) h[6].getIntValue(); p.chordsPerBar = juce::jmax (1, h[7].getIntValue());
    p.lengthTicks = p.bars * kal::TPBAR;
    for (auto& c : juce::StringArray::fromTokens (parts[1], ",", "")) if (c.isNotEmpty()) p.chordRoots.push_back (c.getIntValue());
    for (auto& t : juce::StringArray::fromTokens (parts[2], ";", ""))
    {
        auto v = juce::StringArray::fromTokens (t, ",", ""); if (v.size() < 4) continue;
        p.notes.push_back ({ v[0].getDoubleValue(), v[1].getDoubleValue(), v[2].getIntValue(), (float) v[3].getDoubleValue() });
    }
    p.feat = kal::Generator::measure (p);
    return p.valid();
}
} // namespace

// =============================================================================
BankLibrary::BankLibrary() { formats.registerBasicFormats(); }

juce::Array<juce::File> BankLibrary::roots() const
{
    juce::Array<juce::File> r;
   #if JUCE_MAC
    r.add (juce::File ("/Library/Application Support/Room Labs/Kaleidoscope/Banques"));
    r.add (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support/Room Labs/Kaleidoscope/Banques"));
   #else
    r.add (juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory).getChildFile ("Room Labs/Kaleidoscope/Banques"));
   #endif
    r.add (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Room Labs/Kaleidoscope/Banques"));
    return r;
}

juce::File BankLibrary::folderFor (const juce::String& id) const
{
    if (id.isEmpty()) return {};
    for (auto& r : roots()) { auto d = r.getChildFile (id); if (d.getChildFile ("instrument.json").existsAsFile()) return d; }
    return {};
}

std::unique_ptr<kal::SamplePatch> BankLibrary::load (const juce::String& id, juce::String& error) const
{
    auto dir = folderFor (id);
    if (! dir.isDirectory()) { error = "Fond non installé : lancez l'installeur des fonds Kaleidoscope."; return nullptr; }
    auto meta = juce::JSON::parse (dir.getChildFile ("instrument.json"));
    auto* regions = meta.getProperty ("regions", {}).getArray();
    if (regions == nullptr || regions->isEmpty()) { error = "instrument.json illisible"; return nullptr; }
    auto patch = std::make_unique<kal::SamplePatch>();
    patch->percussive = ! (bool) meta.getProperty ("sustain", false);
    for (auto& rv : *regions)
    {
        auto f = dir.getChildFile (rv.getProperty ("file", "").toString());
        std::unique_ptr<juce::AudioFormatReader> rd (const_cast<juce::AudioFormatManager&> (formats).createReaderFor (f));
        if (rd == nullptr || rd->lengthInSamples <= 0) continue;
        const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 12));
        juce::AudioBuffer<float> buf ((int) juce::jmax (1u, rd->numChannels), len);
        rd->read (&buf, 0, len, 0, true, true);
        kal::SampleRegion r;
        r.root = (int) rv.getProperty ("root", 60); r.lo = (int) rv.getProperty ("lo", 0); r.hi = (int) rv.getProperty ("hi", 127);
        r.vlo = (int) rv.getProperty ("vlo", 0); r.vhi = (int) rv.getProperty ("vhi", 127);
        r.gain = (float) (double) rv.getProperty ("gain", 1.0); r.tune = (float) (double) rv.getProperty ("tune", 0.0);
        r.loop = (bool) rv.getProperty ("loop", false); r.loopStart = (int) rv.getProperty ("loopStart", 0); r.loopEnd = juce::jmin (len, (int) rv.getProperty ("loopEnd", 0));
        r.sr = rd->sampleRate; r.frames = len; r.data.resize ((size_t) len * 2);
        const float* L = buf.getReadPointer (0); const float* R = buf.getReadPointer (buf.getNumChannels() > 1 ? 1 : 0);
        for (int i = 0; i < len; ++i) { r.data[(size_t) i * 2] = L[i]; r.data[(size_t) i * 2 + 1] = R[i]; }
        patch->regions.push_back (std::move (r));
    }
    if (patch->regions.empty()) { error = "Aucun échantillon lisible dans " + dir.getFullPathName(); return nullptr; }
    return patch;
}

// =============================================================================
KaleidoProcessor::KaleidoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Kaleidoscope", kal::params::layout())
{
    auto info = juce::JSON::parse (juce::String::fromUTF8 (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = info.getProperty ("presets", {}).getArray())
        for (auto& v : *list)
        {
            FactoryPreset p;
            p.id = v.getProperty ("id", "").toString(); p.name = v.getProperty ("name", "").toString(); p.cat = v.getProperty ("cat", "").toString();
            p.bank = v.getProperty ("bank", "").toString(); p.fond = (bool) v.getProperty ("fond", false); p.version = v.getProperty ("version", "").toString();
            p.style = indexOf (v.getProperty ("style", "").toString(), true); p.mood = indexOf (v.getProperty ("mood", "").toString(), false);
            if (auto* o = v.getProperty ("params", {}).getDynamicObject()) p.values = o->getProperties();
            presets.push_back (std::move (p));
        }
    for (auto& f : kFields) soundRaw.push_back ({ f.id, state.getRawParameterValue (f.id) });
    pPlay = state.getRawParameterValue ("play"); pFollow = state.getRawParameterValue ("follow"); pTempo = state.getRawParameterValue ("tempo");
    pHuman = state.getRawParameterValue ("humanize"); pGate = state.getRawParameterValue ("gate"); pMidiOut = state.getRawParameterValue ("midiOut");
    pEvolve = state.getRawParameterValue ("evolve");

    auto txt = profileFile().loadFileAsString();
    if (txt.isNotEmpty()) profile.fromString (txt.toStdString());
    if (! presets.empty()) loadPreset (0);
    generate();                                   // une première proposition, prête à jouer
    startTimerHz (30);
}

KaleidoProcessor::~KaleidoProcessor()
{
    stopTimer();
    loader.removeAllJobs (true, 4000);
    engine.setPatch (nullptr);
}

bool KaleidoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void KaleidoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);     // allocations (tables, écho, réverbération) ici
    keyboard.reset();
    nOffs = 0; wasPlaying = false; lastEnd = -1;
}

void KaleidoProcessor::readSound (kal::SynthParams& p) const
{
    for (int k = 0; k < kNumFields; ++k)
    {
        const float v = soundRaw[(size_t) k].second->load (std::memory_order_relaxed);
        const auto& f = kFields[k];
        if (f.f) p.*(f.f) = v; else if (f.i) p.*(f.i) = (int) std::lround (v); else p.*(f.b) = v > 0.5f;
    }
}

int KaleidoProcessor::choiceIndex (const char* id) const { return (int) std::lround (state.getRawParameterValue (id)->load()); }
float KaleidoProcessor::value (const char* id) const { return state.getRawParameterValue (id)->load(); }

// -----------------------------------------------------------------------------
void KaleidoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);

    bool hostPlaying = false; double ppq = 0; bool ppqOk = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) hostBpm = *bpm;
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; ppqOk = true; }
        }
    const bool follow = pFollow->load() > 0.5f;
    const double bpm = (follow && hostPlaying) ? hostBpm.load() : (double) pTempo->load();
    engine.setTempo (bpm);
    kal::SynthParams sp; readSound (sp); engine.setParams (sp);

    // événements du bloc : MIDI entrant + séquenceur, joués à l'échantillon près
    struct Ev { int at; int type; int pitch; float vel; };      // type 0 off, 1 on, 2 bend, 3 tout couper
    std::array<Ev, 512> evs; int ne = 0;
    auto push = [&] (int at, int type, int pitch, float vel) { if (ne < (int) evs.size()) evs[(size_t) ne++] = { juce::jlimit (0, juce::jmax (0, n - 1), at), type, pitch, vel }; };
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) push (meta.samplePosition, 1, m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) push (meta.samplePosition, 0, m.getNoteNumber(), 0);
        else if (m.isPitchWheel()) push (meta.samplePosition, 2, 0, (m.getPitchWheelValue() - 8192) / 8192.f);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) push (meta.samplePosition, 3, 0, 0);
    }
    midi.clear();
    const bool midiOut = pMidiOut->load() > 0.5f;
    std::array<Ev, 256> seq; int ns = 0;
    auto seqPush = [&] (int at, int type, int pitch, float vel) { if (ns < (int) seq.size()) seq[(size_t) ns++] = { juce::jlimit (0, juce::jmax (0, n - 1), at), type, pitch, vel }; };
    auto flushOffs = [&] (int at) { for (int i = 0; i < nOffs; ++i) seqPush (at, 0, offs[(size_t) i].pitch, 0); nOffs = 0; };

    const Playable* pat = live.load (std::memory_order_acquire);
    const bool play = pPlay->load() > 0.5f && pat != nullptr && ! pat->notes.empty();
    if (! play)
    {
        if (wasPlaying) { flushOffs (0); wasPlaying = false; lastLoop = -1; }
        seqStep = -1;
    }
    else
    {
        const double bps = bpm / 60.0 / getSampleRate();
        const bool useHost = follow && hostPlaying && ppqOk;
        if (! useHost && ! wasPlaying) freeBeat = 0;
        const double b0 = useHost ? ppq : freeBeat, b1 = b0 + n * bps;
        if (wasPlaying && std::abs (b0 - lastEnd) > 1e-3) { flushOffs (0); lastLoop = -1; }   // saut de position (boucle de l'hôte, relocalisation)
        auto atOf = [&] (double b) { return (int) std::floor ((b - b0) / bps); };
        for (int i = 0; i < nOffs;)
            if (offs[(size_t) i].beat < b1) { seqPush (atOf (offs[(size_t) i].beat), 0, offs[(size_t) i].pitch, 0); offs[(size_t) i] = offs[(size_t) --nOffs]; }
            else ++i;
        const double L = pat->lengthBeats, hum = pHuman->load(), gate = pGate->load();
        const long k0 = (long) std::floor (b0 / L), k1 = (long) std::floor (b1 / L);
        for (long k = k0; k <= k1; ++k)
            for (auto& e : pat->notes)
            {
                humRng ^= humRng << 13; humRng ^= humRng >> 17; humRng ^= humRng << 5;
                const double r = (humRng & 0xFFFF) / 65535.0;
                const double t = k * L + e.start + (e.start > 0 ? r * 0.012 * hum : 0.0);
                if (t < b0 || t >= b1) continue;
                const int at = atOf (t);
                for (int i = 0; i < nOffs; ++i) if (offs[(size_t) i].pitch == e.pitch) { seqPush (at, 0, e.pitch, 0); offs[(size_t) i] = offs[(size_t) --nOffs]; break; }
                const float vel = (float) juce::jlimit (0.05, 1.0, e.vel * (1.0 + (r - 0.5) * 0.25 * hum));
                seqPush (at, 1, e.pitch, vel);
                if (nOffs < (int) offs.size()) offs[(size_t) nOffs++] = { t + juce::jmax (0.06, e.len * gate), e.pitch };
            }
        const int loop = (int) k1;
        if (loop != lastLoop) { if (lastLoop >= 0) loopsDone.fetch_add (1); lastLoop = loop; }
        beatPos = std::fmod (b1, L);
        seqStep = (int) (std::fmod (b1, L) * 4);
        playingSerial = pat->serial;
        freeBeat = b1; lastEnd = b1; wasPlaying = true;
    }
    for (int i = 0; i < ns; ++i)
    {
        push (seq[(size_t) i].at, seq[(size_t) i].type, seq[(size_t) i].pitch, seq[(size_t) i].vel);
        if (midiOut)
        {
            const auto& e = seq[(size_t) i];
            midi.addEvent (e.type == 1 ? juce::MidiMessage::noteOn (1, e.pitch, juce::jlimit (0.01f, 1.f, e.vel)) : juce::MidiMessage::noteOff (1, e.pitch), e.at);
        }
    }
    std::stable_sort (evs.begin(), evs.begin() + ne, [] (const Ev& a, const Ev& b) { return a.at < b.at; });

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    int at0 = 0;
    for (int i = 0; i < ne; ++i)
    {
        const auto& e = evs[(size_t) i];
        if (e.at > at0) { engine.render (l + at0, r + at0, e.at - at0); at0 = e.at; }
        switch (e.type)
        {
            case 0: engine.noteOff (e.pitch); break;
            case 1: engine.noteOn (e.pitch, e.vel); break;
            case 2: engine.pitchBend (e.vel); break;
            default: engine.allNotesOff(); break;
        }
    }
    if (at0 < n) engine.render (l + at0, r + at0, n - at0);
}

// -----------------------------------------------------------------------------
kal::Request KaleidoProcessor::makeRequest() const
{
    kal::Request r;
    r.style = choiceIndex ("style"); r.mood = choiceIndex ("mood");
    r.mode = choiceIndex ("genMode") == 1 ? kal::Mode::Accomp : kal::Mode::Solo;
    r.root = choiceIndex ("root"); r.scale = choiceIndex ("scale") - 1;
    static const int bars[] = { 1, 2, 4, 8 }; r.bars = bars[juce::jlimit (0, 3, choiceIndex ("bars"))];
    r.density = value ("density"); r.octave = (int) std::lround (value ("octave"));
    r.variation = value ("variation"); r.personal = value ("personal");
    return r;
}

void KaleidoProcessor::install (const kal::Pattern& p, bool rememberIt)
{
    if (hasPattern) { past.push_back (current); while (past.size() > 32) past.pop_front(); }
    current = p; hasPattern = true;
    if (rememberIt) gen.remember (p);
    auto pl = std::make_unique<Playable>();
    for (auto& nt : p.notes) pl->notes.push_back ({ nt.start / kal::TPB, nt.len / kal::TPB, nt.pitch, nt.vel });
    std::sort (pl->notes.begin(), pl->notes.end(), [] (auto& a, auto& b) { return a.start < b.start; });
    pl->lengthBeats = p.lengthTicks / kal::TPB; pl->serial = ++serialCounter;
    live.store (pl.get(), std::memory_order_release);
    if (owned) retired.push_back ({ std::move (owned), juce::Time::getMillisecondCounterHiRes() });
    owned = std::move (pl);
    patternSerial.fetch_add (1);
}

juce::var KaleidoProcessor::generate()
{
    auto r = makeRequest();
    install (gen.generate (r, value ("personal") > 0.01f ? &profile : nullptr), true);
    return patternInfo();
}

juce::var KaleidoProcessor::decline()
{
    if (! hasPattern) return generate();
    auto r = makeRequest();
    install (gen.decline (current, r, value ("personal") > 0.01f ? &profile : nullptr), true);
    return patternInfo();
}

juce::var KaleidoProcessor::previous()
{
    if (past.empty()) return patternInfo();
    auto p = past.back(); past.pop_back();
    hasPattern = false;                                    // ne pas réempiler la proposition qu'on quitte
    install (p, false);
    return patternInfo();
}

juce::var KaleidoProcessor::feedback (const juce::String& kind)
{
    if (! hasPattern) return patternInfo();
    if (kind == "like") profile.like (current.feat, 1.0);
    else if (kind == "keep") profile.like (current.feat, 2.0);
    else if (kind == "redo") profile.dislike (current.feat);
    profileFile().getParentDirectory().createDirectory();
    profileFile().replaceWithText (profile.toString());
    if (kind == "redo") return generate();
    return patternInfo();
}

void KaleidoProcessor::resetPersonal()
{
    profile.reset(); gen.clearHistory();
    profileFile().deleteFile();
}

juce::File KaleidoProcessor::profileFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile (juce::String ("Room Labs") + juce::File::getSeparatorString() + "Kaleidoscope").getChildFile ("personal.txt");
}

juce::var KaleidoProcessor::setDrop (const juce::var& a)
{
    auto set = [&] (const char* id, float v) { if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id)))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture(); } };
    set ("keyMode", 1);
    if (a.hasProperty ("root")) set ("root", (float) juce::jlimit (0, 11, (int) a["root"]));
    if (a.hasProperty ("scale")) set ("scale", (float) juce::jlimit (0, 20, (int) a["scale"] + 1));
    if (a.hasProperty ("bpm") && (double) a["bpm"] > 40) set ("tempo", (float) juce::jlimit (50.0, 200.0, (double) a["bpm"]));
    return generate();
}

juce::File KaleidoProcessor::exportMidi (juce::File dest)
{
    if (! hasPattern) return {};
    juce::MidiMessageSequence seq;
    const double tpq = 960, bpm = pFollow->load() > 0.5f ? hostBpm.load() : (double) pTempo->load();
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    for (auto& nt : current.notes)
    {
        const double s = nt.start / kal::TPB * tpq, e = s + juce::jmax (0.06, nt.len / kal::TPB * pGate->load()) * tpq;
        seq.addEvent (juce::MidiMessage::noteOn (1, nt.pitch, juce::jlimit (0.05f, 1.f, nt.vel)), s);
        seq.addEvent (juce::MidiMessage::noteOff (1, nt.pitch), e);
    }
    seq.updateMatchedPairs(); seq.sort();
    juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) tpq); mf.addTrack (seq);
    if (dest == juce::File())
    {
        static const char* rootNames[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Room Labs/Kaleidoscope/MIDI");
        dir.createDirectory();
        const juce::String name = juce::String ("Kaleidoscope_") + kal::data::kStyles[current.style].id + "_" + rootNames[(current.root % 12 + 12) % 12]
            + "_" + juce::String ((int) std::lround (bpm)) + "bpm_" + juce::String::toHexString ((juce::int64) current.seed).substring (0, 6) + ".mid";
        dest = dir.getChildFile (name);
    }
    dest.deleteFile();
    juce::FileOutputStream out (dest);
    if (! out.openedOk() || ! mf.writeTo (out)) return {};
    return dest;
}

juce::var KaleidoProcessor::patternInfo() const
{
    auto* o = new juce::DynamicObject();
    if (! hasPattern) return juce::var (o);
    o->setProperty ("serial", patternSerial.load());
    o->setProperty ("seed", juce::String::toHexString ((juce::int64) current.seed));
    o->setProperty ("style", current.style); o->setProperty ("mood", current.mood);
    o->setProperty ("root", current.root); o->setProperty ("scale", current.scale);
    o->setProperty ("bars", current.bars); o->setProperty ("mode", (int) current.mode);
    o->setProperty ("beats", current.lengthTicks / kal::TPB);
    juce::Array<juce::var> notes;
    for (auto& nt : current.notes) notes.add (juce::Array<juce::var> { nt.start / kal::TPB, nt.len / kal::TPB, nt.pitch, nt.vel });
    o->setProperty ("notes", notes);
    juce::Array<juce::var> feat; for (int i = 0; i < kal::Features::N; ++i) feat.add (current.feat[i]);
    o->setProperty ("feat", feat);
    o->setProperty ("novelty", gen.lastNovelty); o->setProperty ("coherence", gen.lastCoherence);
    o->setProperty ("history", (int) past.size());
    o->setProperty ("liked", profile.nPos); o->setProperty ("disliked", profile.nNeg);
    o->setProperty ("affinity", profile.nPos + profile.nNeg > 0 ? profile.score (current.feat) : 0.0);
    return juce::var (o);
}

juce::var KaleidoProcessor::bankInfo() const
{
    auto* o = new juce::DynamicObject();
    juce::Array<juce::var> inst;
    std::set<juce::String> seen;
    for (auto& p : presets) if (p.fond && ! seen.count (p.bank)) { seen.insert (p.bank); if (banks.installed (p.bank)) inst.add (p.bank); }
    o->setProperty ("installed", inst);
    o->setProperty ("total", (int) seen.size());
    const juce::ScopedLock sl (bankLock);
    o->setProperty ("loaded", bankId); o->setProperty ("wanted", wantedBank); o->setProperty ("error", bankErr);
    o->setProperty ("loading", bankJobs.load() > 0);
    o->setProperty ("folder", banks.roots()[0].getFullPathName());
    return juce::var (o);
}

void KaleidoProcessor::requestBank (const juce::String& id)
{
    {
        const juce::ScopedLock sl (bankLock);
        if (id == bankId && patch) return;
        wantedBank = id; bankErr = {};
    }
    bankJobs.fetch_add (1);
    loader.addJob ([this, id]
    {
        juce::String err;
        auto p = banks.load (id, err);
        const juce::ScopedLock sl (bankLock);
        if (wantedBank == id)
        {
            if (p)
            {
                engine.setPatch (p.get());
                if (patch) oldPatches.push_back ({ std::move (patch), juce::Time::getMillisecondCounterHiRes() });
                patch = std::move (p); bankId = id; bankErr = {};
            }
            else bankErr = err;
        }
        bankJobs.fetch_sub (1);
    });
}

// -----------------------------------------------------------------------------
const juce::String KaleidoProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return (presets[(size_t) index].fond ? "Fond - " : "") + presets[(size_t) index].cat + " - " + presets[(size_t) index].name;
    return {};
}

void KaleidoProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    const auto& fp = presets[(size_t) index];
    for (auto& kv : fp.values)
    {
        auto* prm = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (kv.name.toString()));
        if (prm == nullptr) continue;
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (double) kv.value));
        prm->endChangeGesture();
    }
    if (fp.fond) requestBank (fp.bank);
}

void KaleidoProcessor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    retired.erase (std::remove_if (retired.begin(), retired.end(), [&] (auto& r) { return now - r.second > 3000; }), retired.end());
    {
        const juce::ScopedLock sl (bankLock);
        oldPatches.erase (std::remove_if (oldPatches.begin(), oldPatches.end(), [&] (auto& r) { return now - r.second > 5000; }), oldPatches.end());
    }
    // mode infini : une déclinaison toutes les N boucles, une idée neuve toutes les 4 évolutions
    static const int every[] = { 0, 1, 2, 4, 8 };
    const int ev = every[juce::jlimit (0, 4, (int) std::lround (pEvolve->load()))];
    const int done = loopsDone.load();
    if (ev > 0 && pPlay->load() > 0.5f)
    {
        if (done - loopsSeen >= ev)
        {
            loopsSeen = done;
            if (++evolveCounter % 4 == 0) generate(); else decline();
        }
    }
    else loopsSeen = done;
}

juce::var KaleidoProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}

void KaleidoProcessor::setProjectUi (const juce::String& key, const juce::var& v)
{
    const juce::ScopedLock sl (uiLock);
    projectUi.set (key, v);
}

void KaleidoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("preset", currentPreset.load(), nullptr);
    tree.setProperty ("version", KAL_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    tree.setProperty ("bank", loadedBank(), nullptr);
    if (hasPattern) tree.setProperty ("pattern", patternToString (current), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void KaleidoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasType (state.state.getType())) return;
    currentPreset = juce::jlimit (0, juce::jmax (0, (int) presets.size() - 1), (int) tree.getProperty ("preset", 0));
    const auto ui = juce::JSON::parse (tree.getProperty ("ui", "{}").toString());
    if (auto* o = ui.getDynamicObject()) { const juce::ScopedLock sl (uiLock); projectUi = o->getProperties(); }
    const auto bank = tree.getProperty ("bank", "").toString();
    kal::Pattern p;
    const bool okPat = patternFromString (tree.getProperty ("pattern", "").toString(), p);
    for (auto* k : { "preset", "version", "ui", "bank", "pattern" }) tree.removeProperty (k, nullptr);
    state.replaceState (tree);
    if (bank.isNotEmpty()) requestBank (bank);
    if (okPat) { hasPattern = false; install (p, true); }     // la mélodie du projet revient telle quelle
}

bool KaleidoProcessor::hasEditor() const
{
   #if KAL_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* KaleidoProcessor::createEditor()
{
   #if KAL_HEADLESS
    return nullptr;
   #else
    return new KaleidoEditor (*this);
   #endif
}

#if ! KAL_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KaleidoProcessor(); }
#endif
