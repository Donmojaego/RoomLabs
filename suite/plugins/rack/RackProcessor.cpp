// 7 Room Rack — 3 emplacements chaînés (Noise, Reverb, Loop Remix Lab), macros et mix par emplacement.
#include "RLCommon.h"
#include "RackDSP.h"
namespace {
using APF = juce::AudioParameterFloat; using PID = juce::ParameterID; using NR = juce::NormalisableRange<float>;
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    const float defA[3] = { .5f, .4f, .3f };
    for (int k = 1; k <= 3; ++k)
    {
        const juce::String n (k), s = "Slot " + n + " ";
        l.add (std::make_unique<juce::AudioParameterChoice> (PID { "m" + n, 1 }, s + "Module", juce::StringArray { "Vide", "Noise", "Reverb", "Loop Remix Lab" }, k == 1 ? 2 : 0));
        l.add (std::make_unique<juce::AudioParameterBool> (PID { "on" + n, 1 }, s + "On", true));
        l.add (std::make_unique<APF> (PID { "a" + n, 1 }, s + "Macro 1", NR (0.f, 1.f, 0.001f), defA[k - 1]));
        l.add (std::make_unique<APF> (PID { "b" + n, 1 }, s + "Macro 2", NR (0.f, 1.f, 0.001f), 0.5f));
        l.add (std::make_unique<APF> (PID { "c" + n, 1 }, s + "Macro 3", NR (0.f, 1.f, 0.001f), 0.5f));
        l.add (std::make_unique<APF> (PID { "x" + n, 1 }, s + "Mix", NR (0.f, 1.f, 0.001f), k == 1 ? 0.3f : 1.f));
    }
    l.add (std::make_unique<APF> (PID { "out", 1 }, "Output", NR (-24.f, 6.f, 0.1f), 0.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}
}
class RackProcessor : public rl::Processor
{
public:
    RackProcessor() : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true), makeLayout(), "rack/chassis") {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { const auto o = l.getMainOutputChannelSet(); return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o; }
    void prepareToPlay (double sr, int bs) override { eng.prepare (sr, juce::jmax (64, bs)); maxBlock = juce::jmax (64, bs); }
    double getTailLengthSeconds() const override { return 20.0; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        rl::rack::Params q;
        for (int k = 0; k < 3; ++k)
        {
            const juce::String n (k + 1); auto& s = q.slot[(size_t) k];
            s.module = (int) p (("m" + n).toRawUTF8()); s.on = p (("on" + n).toRawUTF8()) > .5f;
            s.a = p (("a" + n).toRawUTF8()); s.b = p (("b" + n).toRawUTF8()); s.c = p (("c" + n).toRawUTF8()); s.mix = p (("x" + n).toRawUTF8());
        }
        q.outDb = p ("out"); eng.setParams (q);
        double bpm = 120, ppq = 0; bool play = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
        { if (auto v = pos->getBpm()) bpm = *v; if (auto v = pos->getPpqPosition()) ppq = *v; play = pos->getIsPlaying(); }
        const int total = b.getNumSamples(), ch = b.getNumChannels();
        for (int o = 0; o < total; o += maxBlock)
        {
            const int n = juce::jmin (maxBlock, total - o);
            float* io[2] = { b.getWritePointer (0, o), b.getWritePointer (ch > 1 ? 1 : 0, o) };
            eng.setTransport (bpm, ppq + o * bpm / 60.0 / getSampleRate(), play);
            eng.process (io, ch, n);
        }
        licenseGate (b);
    }
    juce::var feedback() override
    {
        auto* o = new juce::DynamicObject(); auto db = [] (float a) { return a > 1e-6f ? 20.f * std::log10 (a) : -120.f; };
        juce::Array<juce::var> l; for (auto& v : eng.meter.lvl) l.add (db (v.load())); o->setProperty ("lvl", l); o->setProperty ("out", db (eng.meter.out.load()));
        return juce::var (o);
    }
private:
    rl::rack::Engine eng; int maxBlock = 512;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RackProcessor(); }
