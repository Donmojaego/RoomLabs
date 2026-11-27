// Loop Remix Lab — remix en temps réel sur 16 pas calés sur le tempo, 16 effets, 2 LFO.
#include "RLCommon.h"
#include "RemixDSP.h"
namespace {
using APF = juce::AudioParameterFloat; using APC = juce::AudioParameterChoice; using PID = juce::ParameterID; using NR = juce::NormalisableRange<float>;
const juce::StringArray kFx { "Thru", "Repeat", "Roll", "Reverse", "Half Speed", "Tape Stop", "Scratch", "Gate", "Mute", "Crush", "Filter Sweep", "Ring", "Echo", "Pan", "Pitch Up", "Drift" };
const juce::StringArray kStyles { "Glitch", "Stutter", "Tape", "Dub", "Lo-Fi", "Trap Roll", "Reverse", "Scratch", "Gate", "Ambient", "Chaos", "Minimal" };
const juce::StringArray kShapes { "Sinus", "Triangle", "Scie", "Carré", "Aléatoire" };
const juce::StringArray kRates { "4 mesures", "2 mesures", "1 mesure", "1/2", "1/4", "1/8", "1/16" };
const juce::StringArray kDest { "Filtre", "Volume", "Pan", "Crush", "Ring", "Résonance" };
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (int i = 0; i < 16; ++i) l.add (std::make_unique<APC> (PID { "s" + juce::String (i), 1 }, "Step " + juce::String (i + 1), kFx, 0));
    l.add (std::make_unique<APC> (PID { "style", 1 }, "Style", kStyles, 0));
    l.add (std::make_unique<APF> (PID { "density", 1 }, "Density", NR (0.f, 1.f, 0.001f), 0.5f));
    NR fr (20.f, 20000.f, 0.f); fr.setSkewForCentre (1000.f);
    l.add (std::make_unique<APF> (PID { "cutoff", 1 }, "Filter", fr, 20000.f, juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<APF> (PID { "reso", 1 }, "Resonance", NR (0.f, 0.95f, 0.001f), 0.2f));
    l.add (std::make_unique<APF> (PID { "mix", 1 }, "Mix", NR (0.f, 1.f, 0.001f), 1.f));
    l.add (std::make_unique<APF> (PID { "out", 1 }, "Output", NR (-24.f, 6.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    for (int k = 1; k <= 2; ++k)
    {
        const juce::String n = "l" + juce::String (k);
        l.add (std::make_unique<APC> (PID { n + "shape", 1 }, "LFO" + juce::String (k) + " Shape", kShapes, 0));
        l.add (std::make_unique<APC> (PID { n + "rate", 1 }, "LFO" + juce::String (k) + " Rate", kRates, k == 1 ? 3 : 2));
        l.add (std::make_unique<APF> (PID { n + "depth", 1 }, "LFO" + juce::String (k) + " Depth", NR (0.f, 1.f, 0.001f), 0.f));
        l.add (std::make_unique<APC> (PID { n + "dest", 1 }, "LFO" + juce::String (k) + " Dest", kDest, k == 1 ? 0 : 2));
    }
    return l;
}
}
class RemixProcessor : public rl::Processor
{
public:
    RemixProcessor() : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true), makeLayout(), "vst/loop_remix_lab") {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { const auto o = l.getMainOutputChannelSet(); return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o; }
    void prepareToPlay (double sr, int) override { eng.prepare (sr); }
    double getTailLengthSeconds() const override { return 0; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        rl::rx::Params q;
        for (int i = 0; i < 16; ++i) q.steps[(size_t) i] = (int) p (("s" + juce::String (i)).toRawUTF8());
        q.cutoff = p ("cutoff"); q.reso = p ("reso"); q.mix = p ("mix"); q.outDb = p ("out");
        q.lfo[0] = { (int) p ("l1shape"), (int) p ("l1rate"), p ("l1depth"), (int) p ("l1dest") };
        q.lfo[1] = { (int) p ("l2shape"), (int) p ("l2rate"), p ("l2depth"), (int) p ("l2dest") };
        eng.setParams (q);
        double bpm = 120, ppq = 0; bool play = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
        { if (auto v = pos->getBpm()) bpm = *v; if (auto v = pos->getPpqPosition()) ppq = *v; play = pos->getIsPlaying(); }
        eng.setTransport (bpm, ppq, play);
        eng.process (b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
        licenseGate (b);
    }
    juce::var feedback() override
    {
        auto* o = new juce::DynamicObject(); auto& m = eng.meter;
        o->setProperty ("st", m.step.load()); o->setProperty ("l1", m.lfo1.load()); o->setProperty ("l2", m.lfo2.load());
        juce::Array<juce::var> w; for (auto& v : m.wave) w.add (v.load()); o->setProperty ("w", w);
        return juce::var (o);
    }
private:
    rl::rx::Engine eng;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RemixProcessor(); }
