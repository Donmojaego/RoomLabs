// Reverb — Room / Hall / Plate ; la queue continue en bypass, changements de type sans clic.
#include "RLCommon.h"
#include "ReverbDSP.h"
namespace {
using APF = juce::AudioParameterFloat; using PID = juce::ParameterID; using NR = juce::NormalisableRange<float>;
NR skew (float a, float b, float c) { NR r (a, b, 0.f); r.setSkewForCentre (c); return r; }
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "type", 1 }, "Type", juce::StringArray { "Room", "Hall", "Plate" }, 1));
    l.add (std::make_unique<APF> (PID { "size", 1 }, "Size", NR (0.f, 1.f, 0.001f), 0.6f));
    l.add (std::make_unique<APF> (PID { "decay", 1 }, "Decay", skew (0.1f, 30.f, 2.5f), 2.4f, juce::AudioParameterFloatAttributes().withLabel ("s")));
    l.add (std::make_unique<APF> (PID { "predelay", 1 }, "Pre-delay", skew (0.f, 250.f, 40.f), 20.f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<APF> (PID { "damp", 1 }, "Damping", skew (1000.f, 20000.f, 6000.f), 7000.f, juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<APF> (PID { "lowcut", 1 }, "Low Cut", skew (20.f, 600.f, 150.f), 120.f, juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<APF> (PID { "mix", 1 }, "Mix", NR (0.f, 1.f, 0.001f), 0.3f));
    l.add (std::make_unique<APF> (PID { "width", 1 }, "Width", NR (0.f, 1.f, 0.001f), 1.f));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "freeze", 1 }, "Bypass (tail)", false));
    return l;
}
}
class ReverbProcessor : public rl::Processor
{
public:
    ReverbProcessor() : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true), makeLayout(), "vst/reverb") {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { const auto o = l.getMainOutputChannelSet(); return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o; }
    void prepareToPlay (double sr, int) override { eng.prepare (sr); }
    double getTailLengthSeconds() const override { return 30.0; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        rl::rv::Params q; q.type = (int) p ("type"); q.size = p ("size"); q.decay = p ("decay"); q.predelayMs = p ("predelay");
        q.dampHz = p ("damp"); q.lowCutHz = p ("lowcut"); q.mix = p ("mix"); q.width = p ("width"); q.bypass = p ("freeze") > .5f;
        eng.setParams (q);
        eng.process (b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
        licenseGate (b);
    }
    juce::var feedback() override
    {
        auto* o = new juce::DynamicObject(); auto& m = eng.meter;
        auto db = [] (float a) { return a > 1e-6f ? 20.f * std::log10 (a) : -120.f; };
        o->setProperty ("in", db (m.inL.load())); o->setProperty ("wet", db (m.wetL.load()));
        juce::Array<juce::var> e; for (auto& v : m.decayEnv) e.add (v.load()); o->setProperty ("env", e);
        return juce::var (o);
    }
private:
    rl::rv::Engine eng;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReverbProcessor(); }
