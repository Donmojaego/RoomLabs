#include "PluginProcessor.h"
#include "Params.h"
#include <BinaryData.h>
#if ! SHK_HEADLESS
 #include "PluginEditor.h"
#endif

static std::vector<FactoryPreset> loadFactory()
{
    std::vector<FactoryPreset> out;
    const auto json = juce::JSON::parse (juce::String::fromUTF8 (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = json.getProperty ("presets", {}).getArray())
        for (auto& v : *list)
        {
            FactoryPreset p;
            p.name = v.getProperty ("name", "").toString();
            p.section = v.getProperty ("section", "").toString();
            p.root = (int) v.getProperty ("root", 29);
            p.rootName = v.getProperty ("rootName", "").toString();
            p.outfit = (int) v.getProperty ("outfit", 0);
            if (auto* o = v.getProperty ("params", {}).getDynamicObject())
                p.values = o->getProperties();
            out.push_back (std::move (p));
        }
    return out;
}

ShakeItProcessor::ShakeItProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ShakeIt", shk::params::layout()),
      presets (loadFactory())
{
    auto get = [this] (const char* id) { return state.getRawParameterValue (id); };
    raw = { get ("tune"), get ("glide"), get ("punch"), get ("decay"), get ("drive"), get ("output"), get ("harm"),
            get ("tone"), get ("reso"), get ("attack"), get ("release"), get ("model"), get ("glideMode"), get ("driveType") };
    if (! presets.empty()) loadPreset (0);
    masterDb = state.getRawParameterValue ("master");
    license.start();
}

bool ShakeItProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ShakeItProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);      // seule allocation : la corde pincée, ici, hors temps réel
    keyboard.reset();
}

void ShakeItProcessor::readParams (shk::Params& p) const
{
    p.model = (shk::Model) juce::jlimit (0, 3, (int) raw.model->load());
    p.tune = raw.tune->load();
    p.glideMs = raw.glide->load();
    p.glideLegatoOnly = (int) raw.glideMode->load() == 1;
    p.punch = raw.punch->load();
    p.decay = raw.decay->load();
    p.drive = raw.drive->load();
    p.driveType = (shk::DriveType) juce::jlimit (0, 2, (int) raw.driveType->load());
    p.outputDb = raw.output->load();
    p.harm = raw.harm->load();
    p.tone = raw.tone->load();
    p.reso = raw.reso->load();
    p.attackMs = raw.attack->load();
    p.releaseMs = raw.release->load();
}

void ShakeItProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
}

void ShakeItProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboard.processNextMidiBuffer (midi, 0, n, true);   // ajoute les notes de l'interface

    shk::Params p;
    readParams (p);
    engine.setParams (p);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        if (at > pos) { engine.render (l + pos, r ? r + pos : nullptr, at - pos); pos = at; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) engine.render (l + pos, r ? r + pos : nullptr, n - pos);
    midi.clear();
    // volume général (0 dB par défaut ; les presets sont calibrés au même niveau)
    if (masterDb) { const float mg = juce::Decibels::decibelsToGain (masterDb->load()); buffer.applyGainRamp (0, buffer.getNumSamples(), lastMaster, mg); lastMaster = mg; }
    // licence : sans licence valide (une fois Moonbase configuré), silence avec un fondu court
    if (! license.allowed()) { if (wasLicensed) buffer.applyGainRamp (0, buffer.getNumSamples(), 1.f, 0.f); else buffer.clear(); wasLicensed = false; }
    else wasLicensed = true;
}

// ---------------------------------------------------------------------------
const juce::String ShakeItProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return presets[(size_t) index].section + " - " + presets[(size_t) index].name;
    return {};
}

void ShakeItProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    for (auto& kv : presets[(size_t) index].values)
        if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (kv.name.toString())))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (double) kv.value));
            prm->endChangeGesture();
        }
}

juce::var ShakeItProcessor::getProjectUi() const
{
    const juce::ScopedLock sl (uiLock);
    auto* o = new juce::DynamicObject();
    for (auto& kv : projectUi) o->setProperty (kv.name, kv.value);
    return juce::var (o);
}

void ShakeItProcessor::setProjectUi (const juce::String& key, const juce::var& value)
{
    const juce::ScopedLock sl (uiLock);
    projectUi.set (key, value);
}

void ShakeItProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty ("preset", currentPreset.load(), nullptr);
    tree.setProperty ("version", SHK_VERSION_STRING, nullptr);
    tree.setProperty ("ui", juce::JSON::toString (getProjectUi(), true), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destData);
}

void ShakeItProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.hasType (state.state.getType())) return;
        currentPreset = juce::jlimit (0, juce::jmax (0, (int) presets.size() - 1), (int) tree.getProperty ("preset", 0));
        const auto ui = juce::JSON::parse (tree.getProperty ("ui", "{}").toString());
        if (auto* o = ui.getDynamicObject())
        {
            const juce::ScopedLock sl (uiLock);
            projectUi = o->getProperties();
        }
        tree.removeProperty ("preset", nullptr);
        tree.removeProperty ("version", nullptr);
        tree.removeProperty ("ui", nullptr);
        state.replaceState (tree);
    }
}

// ---------------------------------------------------------------------------
bool ShakeItProcessor::hasEditor() const
{
   #if SHK_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* ShakeItProcessor::createEditor()
{
   #if SHK_HEADLESS
    return nullptr;
   #else
    return new ShakeItEditor (*this);
   #endif
}

#if ! SHK_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ShakeItProcessor(); }
#endif
