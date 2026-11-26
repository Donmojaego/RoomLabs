// Noise — générateur de bruit (blanc / rose / brun), muet au départ, sans clic.
#include "RLCommon.h"
#include "NoiseDSP.h"
namespace {
using APF = juce::AudioParameterFloat; using PID = juce::ParameterID;
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "on", 1 }, "Noise On", false));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "type", 1 }, "Type", juce::StringArray { "White", "Pink", "Brown" }, 1));
    l.add (std::make_unique<APF> (PID { "level", 1 }, "Level", juce::NormalisableRange<float> (-60.f, 0.f, 0.1f), -18.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<APF> (PID { "color", 1 }, "Color", juce::NormalisableRange<float> (-1.f, 1.f, 0.001f), 0.f));
    l.add (std::make_unique<APF> (PID { "width", 1 }, "Width", juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 1.f));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "thru", 1 }, "Input Thru", true));
    return l;
}
}
class NoiseProcessor : public rl::Processor
{
public:
    NoiseProcessor() : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true), makeLayout(), "vst/noise") {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { const auto o = l.getMainOutputChannelSet(); return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o; }
    void prepareToPlay (double sr, int) override { eng.prepare (sr); }
    double getTailLengthSeconds() const override { return 0; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        rl::nz::Params q; q.on = p ("on") > .5f; q.type = (int) p ("type"); q.levelDb = p ("level"); q.color = p ("color"); q.width = p ("width"); q.keepInput = p ("thru") > .5f;
        eng.setParams (q);
        eng.process (b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
        licenseGate (b);
    }
    juce::var feedback() override
    {
        auto* o = new juce::DynamicObject(); auto& m = eng.meter;
        auto db = [] (float a) { return a > 1e-6f ? 20.f * std::log10 (a) : -120.f; };
        o->setProperty ("l", db (m.rmsL.load())); o->setProperty ("r", db (m.rmsR.load()));
        juce::Array<juce::var> sc; for (auto& v : m.scope) sc.add (v.load()); o->setProperty ("sc", sc);
        return juce::var (o);
    }
private:
    rl::nz::Engine eng;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoiseProcessor(); }
