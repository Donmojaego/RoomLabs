#include "PluginProcessor.h"
#include "Params.h"
#include <BinaryData.h>
#if ! SOUP_HEADLESS
 #include "PluginEditor.h"
#endif

SoupProcessor::SoupProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "SoupeOuselem", soup::params::layout())
{
    info = juce::JSON::parse (juce::String::fromUTF8 (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = info.getProperty ("presets", {}).getArray())
        for (auto& v : *list)
        {
            FactoryPreset p;
            p.name = v.getProperty ("name", "").toString();
            p.sachet = (int) v.getProperty ("sachet", 1);
            p.icon = v.getProperty ("icon", "").toString();
            p.section = v.getProperty ("section", "").toString();
            p.root = (int) v.getProperty ("root", 60);
            p.rootName = v.getProperty ("rootName", "").toString();
            if (auto* o = v.getProperty ("params", {}).getDynamicObject()) p.values = o->getProperties();
            presets.push_back (std::move (p));
        }
    for (size_t i = 0; i < kIds.size(); ++i) rp[i] = state.getRawParameterValue (kIds[i]);
    if (! presets.empty()) loadPreset (0);
}

bool SoupProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SoupProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);      // allocations (historiques des effets, réverbération) ici, hors temps réel
    keyboard.reset();
}

void SoupProcessor::readParams (soup::Params& p) const
{
    int k = 0;
    auto v = [&] { return rp[(size_t) k++]->load(); };
    p.inst = (soup::Inst) (int) v(); p.fx = (soup::Fx) (int) v(); p.division = (int) v(); p.reverseOn = v() > 0.5f; p.stutterOn = v() > 0.5f;
    p.mix = v(); p.feedback = v(); p.stretch = v(); p.tone = v(); p.damp = v(); p.space = v(); p.levelDb = v();
    p.attackMs = v(); p.decayMs = v(); p.sustain = v(); p.releaseMs = v(); p.brightness = v(); p.tune = v();
    p.arpOn = v() > 0.5f; p.arpMode = (int) v(); p.arpOct = (int) v() + 1;
}

void SoupProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);

    double ppqNow = -1.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) hostBpm = *bpm;
            if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) ppqNow = *q;   // effets calés sur la mesure de l'hôte
        }
    hostPpq = ppqNow;
    engine.setTransport (hostBpm.load(), ppqNow);

    soup::Params p;
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
const juce::String SoupProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return presets[(size_t) index].section + " - " + presets[(size_t) index].name;
    return {};
}

void SoupProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    for (auto& kv : presets[(size_t) index].values)
    {
        const auto id = kv.name.toString();
        auto* prm = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id));
        if (prm == nullptr) continue;
        float value = (float) (double) kv.value;
        if (id == "arpOct") value -= 1.f;      // 1..3 -> choix 0..2
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

juce::var SoupProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}

void SoupProcessor::setProjectUi (const juce::String& key, const juce::var& value)
{
    const juce::ScopedLock sl (uiLock);
    projectUi.set (key, value);
}

void SoupProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("preset", currentPreset.load(), nullptr);
    tree.setProperty ("version", SOUP_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void SoupProcessor::setStateInformation (const void* data, int sizeInBytes)
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

bool SoupProcessor::hasEditor() const
{
   #if SOUP_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* SoupProcessor::createEditor()
{
   #if SOUP_HEADLESS
    return nullptr;
   #else
    return new SoupEditor (*this);
   #endif
}

#if ! SOUP_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SoupProcessor(); }
#endif
