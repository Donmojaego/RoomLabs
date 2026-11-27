// Audio to MIDI — sortie MIDI en temps réel + capture des notes exportable en fichier .mid.
#include "RLCommon.h"
#include "A2MDSP.h"
namespace {
using APF = juce::AudioParameterFloat; using API = juce::AudioParameterInt; using PID = juce::ParameterID; using NR = juce::NormalisableRange<float>;
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "mode", 1 }, "Mode", juce::StringArray { "Mélodie", "Batterie" }, 0));
    l.add (std::make_unique<APF> (PID { "thresh", 1 }, "Threshold", NR (-70.f, -10.f, 0.1f), -45.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<APF> (PID { "minlen", 1 }, "Min Length", NR (20.f, 400.f, 1.f), 60.f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<API> (PID { "transpose", 1 }, "Transpose", -24, 24, 0));
    l.add (std::make_unique<API> (PID { "low", 1 }, "Lowest Note", 0, 127, 28));
    l.add (std::make_unique<API> (PID { "high", 1 }, "Highest Note", 0, 127, 96));
    l.add (std::make_unique<APF> (PID { "velsens", 1 }, "Velocity", NR (0.f, 1.f, 0.001f), 0.7f));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "rec", 1 }, "Capture", true));
    return l;
}
struct Cap { double t; int note, vel; };
}
class A2MProcessor : public rl::Processor
{
public:
    A2MProcessor() : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true), makeLayout(), "vst/audio_to_midi") { fifoBuf.resize (4096); cap.reserve (20000); }
    bool isBusesLayoutSupported (const BusesLayout& l) const override { const auto o = l.getMainOutputChannelSet(); return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o; }
    void prepareToPlay (double sr, int) override { eng.prepare (sr); rate = sr; }
    double getTailLengthSeconds() const override { return 0; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals nd;
        rl::a2m::Params q; q.mode = (int) p ("mode"); q.threshDb = p ("thresh"); q.minLenMs = p ("minlen"); q.transpose = (int) p ("transpose");
        q.lowNote = (int) p ("low"); q.highNote = (int) p ("high"); q.velSens = p ("velsens");
        eng.setParams (q);
        midi.clear();
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto v = pos->getBpm()) bpm = *v;
        rl::a2m::Event ev[rl::a2m::Engine::kMaxEvents];
        const int n = eng.process (b.getArrayOfReadPointers(), b.getNumChannels(), b.getNumSamples(), ev);
        const bool rec = p ("rec") > .5f;
        for (int i = 0; i < n; ++i)
        {
            const auto& e = ev[i]; const int off = juce::jlimit (0, b.getNumSamples() - 1, e.offset);
            midi.addEvent (e.vel > 0 ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel) : juce::MidiMessage::noteOff (1, e.note), off);
            if (rec) { int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2); if (n1 > 0) { fifoBuf[(size_t) s1] = { (samples + off) / rate, e.note, e.vel }; fifo.finishedWrite (1); } }
        }
        samples += b.getNumSamples();
        licenseGate (b);
    }
    juce::var feedback() override
    {
        drain();
        auto* o = new juce::DynamicObject(); auto& m = eng.meter;
        o->setProperty ("note", m.note.load()); o->setProperty ("hz", m.hz.load()); o->setProperty ("conf", m.conf.load()); o->setProperty ("db", m.levelDb.load());
        o->setProperty ("t", samples / rate); o->setProperty ("count", (int) cap.size() / 2);
        juce::Array<juce::var> r; const int from = juce::jmax (0, (int) cap.size() - 120);
        for (int i = from; i < (int) cap.size(); ++i) { juce::Array<juce::var> e { cap[(size_t) i].t, cap[(size_t) i].note, cap[(size_t) i].vel }; r.add (e); }
        o->setProperty ("roll", r);
        return juce::var (o);
    }
    juce::var call (const juce::String& name, const juce::Array<juce::var>&) override
    {
        if (name == "clear") { drain(); cap.clear(); return true; }
        if (name == "export")
        {
            drain(); if (cap.empty()) return false;
            chooser = std::make_unique<juce::FileChooser> ("Exporter les notes en MIDI", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Audio to MIDI.mid"), "*.mid");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f != juce::File()) writeMidi (f.withFileExtension ("mid")); });
            return true;
        }
        return {};
    }
private:
    void drain() { int s1, n1, s2, n2; fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2); for (int i = 0; i < n1; ++i) cap.push_back (fifoBuf[(size_t) (s1 + i)]); for (int i = 0; i < n2; ++i) cap.push_back (fifoBuf[(size_t) (s2 + i)]); fifo.finishedRead (n1 + n2); }
    void writeMidi (const juce::File& f)
    {
        juce::MidiMessageSequence seq; const double tpq = 960, t0 = cap.front().t;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0);
        for (auto& c : cap) { const double tk = (c.t - t0) * bpm / 60.0 * tpq; seq.addEvent (c.vel > 0 ? juce::MidiMessage::noteOn (1, c.note, (juce::uint8) c.vel) : juce::MidiMessage::noteOff (1, c.note), tk); }
        seq.updateMatchedPairs();
        juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) tpq); mf.addTrack (seq);
        f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) mf.writeTo (os);
    }
    rl::a2m::Engine eng; double rate = 48000, bpm = 120; juce::int64 samples = 0;
    juce::AbstractFifo fifo { 4096 }; std::vector<Cap> fifoBuf, cap;
    std::unique_ptr<juce::FileChooser> chooser;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new A2MProcessor(); }
