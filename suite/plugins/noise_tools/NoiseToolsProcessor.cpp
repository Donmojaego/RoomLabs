// =============================================================================
// Noise Tools — processeur JUCE (paramètres, latence déclarée, retour visuel).
// Le DSP est dans NoiseToolsDSP (testé hors JUCE par tests/NoiseToolsTest.cpp).
// =============================================================================
#include "RLCommon.h"
#include "NoiseToolsDSP.h"

namespace
{
using APF = juce::AudioParameterFloat; using APB = juce::AudioParameterBool; using APC = juce::AudioParameterChoice;
using PID = juce::ParameterID;

juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto db = juce::AudioParameterFloatAttributes().withLabel ("dB");
    l.add (std::make_unique<APB> (PID { "dn_on", 1 }, "Denoise", true));
    l.add (std::make_unique<APF> (PID { "dn_amount", 1 }, "Denoise Reduction", juce::NormalisableRange<float> (0.f, 30.f, 0.1f), 12.f, db));
    l.add (std::make_unique<APF> (PID { "dn_sens", 1 }, "Denoise Sensitivity", juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<APC> (PID { "hum", 1 }, "Hum", juce::StringArray { "Off", "50 Hz", "60 Hz" }, 0));
    l.add (std::make_unique<APB> (PID { "learn", 1 }, "Learn Noise", false));
    l.add (std::make_unique<APB> (PID { "dc_on", 1 }, "De-click", true));
    l.add (std::make_unique<APF> (PID { "dc_sens", 1 }, "De-click Sensitivity", juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<APF> (PID { "dc_len", 1 }, "De-click Length", juce::NormalisableRange<float> (0.3f, 5.f, 0.01f, 0.6f), 1.5f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<APB> (PID { "dp_on", 1 }, "De-clip", false));
    l.add (std::make_unique<APF> (PID { "dp_sens", 1 }, "De-clip Sensitivity", juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<APF> (PID { "dp_margin", 1 }, "De-clip Headroom", juce::NormalisableRange<float> (-12.f, 0.f, 0.1f), -3.f, db));
    l.add (std::make_unique<APF> (PID { "out", 1 }, "Output", juce::NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f, db));
    l.add (std::make_unique<APB> (PID { "bypass", 1 }, "Bypass", false));
    l.add (std::make_unique<APB> (PID { "compare", 1 }, "A/B Original", false));
    return l;
}

float toDb (float a) { return a > 1e-6f ? 20.f * std::log10 (a) : -120.f; }
} // namespace

class NoiseToolsProcessor : public rl::Processor
{
public:
    NoiseToolsProcessor()
        : rl::Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true),
                         makeLayout(), "vst/noise_tools")
    {
        setLatencySamples (rl::nt::Engine::latency());
    }

    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        const auto o = l.getMainOutputChannelSet();
        return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o;
    }

    void prepareToPlay (double sr, int) override
    {
        engine.prepare (sr, juce::jmax (1, getTotalNumOutputChannels()));
        pushParams();
        setLatencySamples (rl::nt::Engine::latency());
    }
    void reset() override { engine.reset(); }
    double getTailLengthSeconds() const override { return 0.0; }

    void processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        for (int c = getTotalNumInputChannels(); c < buf.getNumChannels(); ++c) buf.clear (c, 0, buf.getNumSamples());
        pushParams();
        engine.process (buf.getArrayOfWritePointers(), buf.getNumChannels(), buf.getNumSamples());
        auto mx = [] (std::atomic<float>& a, float v) { float o = a.load(); while (v > o && ! a.compare_exchange_weak (o, v)) {} };
        mx (pk[0], engine.meter.inL.load()); mx (pk[1], engine.meter.inR.load()); mx (pk[2], engine.meter.outL.load()); mx (pk[3], engine.meter.outR.load());
        licenseGate (buf);
    }

    juce::var feedback() override
    {
        auto* o = new juce::DynamicObject();
        const char* names[] = { "inL", "inR", "outL", "outR" };
        for (int i = 0; i < 4; ++i) o->setProperty (names[i], toDb (pk[i].exchange (0.f)));
        auto& m = engine.meter;
        o->setProperty ("red", m.reductionDb.load());
        o->setProperty ("cps", m.clicksPerSec.load());
        o->setProperty ("clps", m.clipsPerSec.load());
        o->setProperty ("hum", m.humDb.load());
        o->setProperty ("clicks", (juce::int64) engine.totalClicks);
        o->setProperty ("clips", (juce::int64) engine.totalClipRuns);
        juce::Array<juce::var> si, so, nz;
        for (int b = 0; b < 32; ++b) { si.add (toDb (std::sqrt (m.specIn[(size_t) b].load()))); so.add (toDb (std::sqrt (m.specOut[(size_t) b].load()))); nz.add (toDb (std::sqrt (m.noise[(size_t) b].load()))); }
        o->setProperty ("si", si); o->setProperty ("so", so); o->setProperty ("nz", nz);
        return juce::var (o);
    }

    juce::var call (const juce::String& name, const juce::Array<juce::var>&) override
    {
        if (name == "resetStats") { engine.totalClicks = engine.totalClipRuns = 0; return true; }
        return {};
    }

private:
    void pushParams()
    {
        rl::nt::Params q;
        q.dnOn = p ("dn_on") > .5f; q.dnAmount = p ("dn_amount"); q.dnSens = p ("dn_sens"); q.hum = (int) p ("hum"); q.learn = p ("learn") > .5f;
        q.dcOn = p ("dc_on") > .5f; q.dcSens = p ("dc_sens"); q.dcLenMs = p ("dc_len");
        q.dpOn = p ("dp_on") > .5f; q.dpSens = p ("dp_sens"); q.dpMarginDb = p ("dp_margin");
        q.outDb = p ("out"); q.bypass = p ("bypass") > .5f; q.compare = p ("compare") > .5f;
        engine.setParams (q);
    }

    rl::nt::Engine engine;
    std::atomic<float> pk[4] { 0, 0, 0, 0 };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoiseToolsProcessor(); }
