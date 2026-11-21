#include "PluginProcessor.h"
#include "Params.h"
#include <BinaryData.h>
#if ! SMK_HEADLESS
 #include "PluginEditor.h"
#endif

SmokeProcessor::SmokeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Smoke", smk::params::layout())
{
    info = juce::JSON::parse (juce::String::fromUTF8 (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = info.getProperty ("presets", {}).getArray())
        for (auto& v : *list)
        {
            FactoryPreset p;
            p.name = v.getProperty ("name", "").toString();
            p.bank = (int) v.getProperty ("bank", 0);
            p.section = v.getProperty ("section", "").toString();
            p.root = (int) v.getProperty ("root", 60);
            p.rootName = v.getProperty ("rootName", "").toString();
            if (auto* o = v.getProperty ("params", {}).getDynamicObject()) p.values = o->getProperties();
            presets.push_back (std::move (p));
        }
    for (size_t i = 0; i < kIds.size(); ++i) rp[i] = state.getRawParameterValue (kIds[i]);
    if (! presets.empty()) loadPreset (0);
}

bool SmokeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SmokeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);      // allocations (chorus, réverbération) ici, hors temps réel
    keyboard.reset();
}

void SmokeProcessor::readParams (smk::Params& p) const
{
    int k = 0;
    auto v = [&] { return rp[(size_t) k++]->load(); };
    p.model = (smk::Model) juce::jlimit (0, (int) smk::Model::Count - 1, (int) v());
    p.tone = v(); p.space = v(); p.mod = v(); p.dream = v(); p.clean = v(); p.levelDb = v();
    p.attackMs = v(); p.decayMs = v(); p.sustain = v(); p.releaseMs = v(); p.bright = v(); p.tremolo = v();
    p.riseS = v(); p.glideMs = v(); p.chord = (int) v(); p.tune = v();
}

void SmokeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);

    smk::Params p;
    readParams (p);
    engine.setParams (p);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int at0 = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > at0) { engine.render (l + at0, r ? r + at0 : nullptr, at - at0); at0 = at; }
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (at0 < n) engine.render (l + at0, r ? r + at0 : nullptr, n - at0);
    midi.clear();
}

// ---------------------------------------------------------------------------
const juce::String SmokeProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return presets[(size_t) index].section + " - " + presets[(size_t) index].name;
    return {};
}

void SmokeProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    for (auto& kv : presets[(size_t) index].values)
    {
        const auto id = kv.name.toString();
        auto* prm = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id));
        if (prm == nullptr) continue;
        float value = (float) (double) kv.value;
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

juce::var SmokeProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}

void SmokeProcessor::setProjectUi (const juce::String& key, const juce::var& value)
{
    const juce::ScopedLock sl (uiLock);
    projectUi.set (key, value);
}

void SmokeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("preset", currentPreset.load(), nullptr);
    tree.setProperty ("version", SMK_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void SmokeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.hasType (state.state.getType())) return;
        currentPreset = juce::jlimit (0, juce::jmax (0, (int) presets.size() - 1), (int) tree.getProperty ("preset", 0));
        const auto ui = juce::JSON::parse (tree.getProperty ("ui", "{}").toString());
        if (auto* o = ui.getDynamicObject()) { const juce::ScopedLock sl (uiLock); projectUi = o->getProperties(); }
        tree.removeProperty ("preset", nullptr);
        tree.removeProperty ("version", nullptr);
        tree.removeProperty ("ui", nullptr);
        state.replaceState (tree);
    }
}

bool SmokeProcessor::hasEditor() const
{
   #if SMK_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* SmokeProcessor::createEditor()
{
   #if SMK_HEADLESS
    return nullptr;
   #else
    return new SmokeEditor (*this);
   #endif
}

#if ! SMK_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SmokeProcessor(); }
#endif
