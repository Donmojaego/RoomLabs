#include "PluginProcessor.h"
#include "Params.h"
#include <BinaryData.h>
#if ! CDY_HEADLESS
 #include "PluginEditor.h"
#endif

CandyShopProcessor::CandyShopProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "CandyShop", cdy::params::layout())
{
    info = juce::JSON::parse (juce::String::fromUTF8 (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = info.getProperty ("presets", {}).getArray())
        for (auto& v : *list)
        {
            FactoryPreset p;
            p.name = v.getProperty ("name", "").toString();
            p.candy = v.getProperty ("candy", "").toString();
            p.section = v.getProperty ("section", "").toString();
            p.root = (int) v.getProperty ("root", 60);
            p.rootName = v.getProperty ("rootName", "").toString();
            if (auto* o = v.getProperty ("params", {}).getDynamicObject()) p.values = o->getProperties();
            presets.push_back (std::move (p));
        }
    for (size_t i = 0; i < kIds.size(); ++i) rp[i] = state.getRawParameterValue (kIds[i]);
    if (! presets.empty()) loadPreset (0);
}

bool CandyShopProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CandyShopProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);      // allocations (réverbération, chorus) ici, hors temps réel
    keyboard.reset();
}

void CandyShopProcessor::readParams (cdy::Params& p) const
{
    // pointeurs atomiques mis en cache dans le constructeur : ni recherche ni allocation ici
    int k = 0;
    auto v = [&] { return rp[(size_t) k++]->load(); };
    p.attackMs = v(); p.decayMs = v(); p.sustain = v(); p.releaseMs = v(); p.tone = v(); p.cutoff = v(); p.space = v(); p.levelDb = v();
    p.detune = v(); p.fmRatio = v(); p.fmAmount = v(); p.fmDecay = v(); p.noise = v(); p.pitchDrop = v(); p.tune = v(); p.reso = v();
    p.fenv = v(); p.fdecayMs = v(); p.chorus = v(); p.velSens = v(); p.arpGate = v(); p.drumLevelDb = v();
    p.algo = (cdy::Algo) (int) v(); p.wave = (int) v(); p.unison = (int) v() + 1; p.chord = (int) v(); p.arpOn = v() > 0.5f;
    p.arpRate = (int) v(); p.arpMode = (int) v(); p.arpOct = (int) v() + 1; p.kit = (int) v();
}

void CandyShopProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm()) { hostBpm = *bpm; }
    engine.setTempo (hostBpm.load());

    cdy::Params p;
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
        if (m.isNoteOn()) engine.noteOn (m.getChannel(), m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.noteOff (m.getChannel(), m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (at0 < n) engine.render (l + at0, r ? r + at0 : nullptr, n - at0);
    midi.clear();
}

// ---------------------------------------------------------------------------
const juce::String CandyShopProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return presets[(size_t) index].section + " - " + presets[(size_t) index].name;
    return {};
}

void CandyShopProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    for (auto& kv : presets[(size_t) index].values)
    {
        const auto id = kv.name.toString();
        auto* prm = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (id));
        if (prm == nullptr) continue;
        float value = (float) (double) kv.value;
        if (id == "unison" || id == "arpOct") value -= 1.f;      // 1..3 -> choix 0..2
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

juce::var CandyShopProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}

void CandyShopProcessor::setProjectUi (const juce::String& key, const juce::var& value)
{
    const juce::ScopedLock sl (uiLock);
    projectUi.set (key, value);
}

void CandyShopProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("preset", currentPreset.load(), nullptr);
    tree.setProperty ("version", CDY_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void CandyShopProcessor::setStateInformation (const void* data, int sizeInBytes)
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

bool CandyShopProcessor::hasEditor() const
{
   #if CDY_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* CandyShopProcessor::createEditor()
{
   #if CDY_HEADLESS
    return nullptr;
   #else
    return new CandyShopEditor (*this);
   #endif
}

#if ! CDY_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CandyShopProcessor(); }
#endif
