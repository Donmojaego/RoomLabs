#include "PluginProcessor.h"
#include "Params.h"
#if ! AFR_HEADLESS
 #include "PluginEditor.h"
#endif

static constexpr const char* kIds[9] = { "instrument", "volume", "attack", "release", "octave", "tune", "tone", "space", "velsens" };

AfricanProcessor::AfricanProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "AfricanRiddim", afr::params::layout())
{
    for (size_t i = 0; i < rp.size(); ++i) rp[i] = state.getRawParameterValue (kIds[i]);
    license.start();
}

bool AfricanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AfricanProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    keyboard.reset();
}

void AfricanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);

    afr::Params p;
    p.instrument = (int) rp[0]->load(); p.volumeDb = rp[1]->load(); p.attackMs = rp[2]->load(); p.releaseMs = rp[3]->load();
    p.octave = (int) rp[4]->load(); p.tune = std::round (rp[5]->load()); p.tone = rp[6]->load(); p.space = rp[7]->load(); p.velSens = rp[8]->load();
    if (p.instrument != lastInstrument) { if (lastInstrument >= 0) engine.allNotesOff(); lastInstrument = p.instrument; }
    engine.setParams (p);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;
    int at0 = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > at0) { engine.process (l + at0, r + at0, at - at0); at0 = at; }
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
        else if (m.isPitchWheel()) engine.pitchBend ((m.getPitchWheelValue() - 8192) / 8192.f * 2.f);
        else if (m.isControllerOfType (1)) engine.modWheel (m.getControllerValue() / 127.f);
    }
    if (at0 < n) engine.process (l + at0, r + at0, n - at0);
    midi.clear();
    // licence : sans licence valide (une fois Moonbase configuré), silence
    if (! license.allowed()) { if (wasLicensed) buffer.applyGainRamp (0, n, 1.f, 0.f); else buffer.clear(); wasLicensed = false; }
    else wasLicensed = true;
}

int AfricanProcessor::getCurrentProgram() { return (int) rp[0]->load(); }
void AfricanProcessor::setCurrentProgram (int index)
{
    if (auto* prm = state.getParameter ("instrument"))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (0, 53, index)));
}
const juce::String AfricanProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, 54)) return {};
    const auto& i = afr::kInst[index];
    return juce::String::fromUTF8 (i.name) + " (" + juce::String::fromUTF8 (i.country) + ")";
}

juce::var AfricanProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}
void AfricanProcessor::setProjectUi (const juce::String& key, const juce::var& value) { const juce::ScopedLock sl (uiLock); projectUi.set (key, value); }

void AfricanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("version", AFR_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void AfricanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.hasType (state.state.getType())) return;
        const auto ui = juce::JSON::parse (tree.getProperty ("ui", "{}").toString());
        if (auto* o = ui.getDynamicObject()) { const juce::ScopedLock sl (uiLock); projectUi = o->getProperties(); }
        tree.removeProperty ("version", nullptr); tree.removeProperty ("ui", nullptr);
        state.replaceState (tree);
    }
}

bool AfricanProcessor::hasEditor() const
{
   #if AFR_HEADLESS
    return false;
   #else
    return true;
   #endif
}
juce::AudioProcessorEditor* AfricanProcessor::createEditor()
{
   #if AFR_HEADLESS
    return nullptr;
   #else
    return new AfricanEditor (*this);
   #endif
}
#if ! AFR_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AfricanProcessor(); }
#endif
