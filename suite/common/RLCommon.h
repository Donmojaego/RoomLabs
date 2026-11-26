#pragma once
// =============================================================================
// Room Labs Audio — socle commun des plugins de la suite.
//  - RLProcessor : AudioProcessor + APVTS, état (paramètres + valeurs d'interface), retour vers l'interface.
//  - RLEditor    : interface WebView (ui/ commun + face du produit), thème Vert/Studio/Pocket, zoom.
// Chaque plugin fournit : ses paramètres, son DSP, un objet JSON de retour (fb) et des fonctions natives.
// =============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "RLLicense.h"
#include <functional>
#include <map>
#include <set>

namespace rl
{
// préférences d'interface communes à toute la suite (thème, zoom) — jamais dans le son
struct Prefs
{
    Prefs()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "Suite"; o.folderName = "Room Labs" + juce::File::getSeparatorString() + "Suite";
        o.filenameSuffix = "settings"; o.osxLibrarySubFolder = "Application Support";
        props = std::make_unique<juce::PropertiesFile> (o);
    }
    juce::PropertiesFile& file() { return *props; }
    std::unique_ptr<juce::PropertiesFile> props;
};

class Processor : public juce::AudioProcessor
{
public:
    Processor (const BusesProperties& b, juce::AudioProcessorValueTreeState::ParameterLayout layout, const juce::String& productId)
        : AudioProcessor (b), state (*this, nullptr, "RoomLabs", std::move (layout)), product (productId) { license.start(); }

    rl::License license;                             // Moonbase (réglages : plugins/<dossier>/licensing.json)
    // à appeler à la fin de processBlock : silence (avec fondu) tant qu'aucune licence valide n'est active
    void licenseGate (juce::AudioBuffer<float>& b)
    {
        if (! license.allowed()) { if (licOk) b.applyGainRamp (0, b.getNumSamples(), 1.f, 0.f); else b.clear(); licOk = false; }
        else licOk = true;
    }
    bool licOk = true;

    juce::AudioProcessorValueTreeState state;
    const juce::String product;                      // ex. "vst/noise_tools"

    // ---- à fournir par chaque plugin
    virtual juce::var feedback() { return {}; }      // retour visuel (~30 Hz) : vumètres, courbes
    virtual juce::var call (const juce::String&, const juce::Array<juce::var>&) { return {}; }   // fonctions propres au plugin

    // ---- valeurs d'interface rangées dans le projet (ex. thème choisi pour ce projet)
    juce::var getUi() const { const juce::ScopedLock sl (uiLock); auto* o = new juce::DynamicObject(); for (auto& kv : ui) o->setProperty (kv.name, kv.value); return juce::var (o); }
    void setUi (const juce::String& k, const juce::var& v) { const juce::ScopedLock sl (uiLock); ui.set (k, v); }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput != 0; }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput != 0; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void releaseResources() override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        auto t = state.copyState();
        t.setProperty ("ui", juce::JSON::toString (getUi(), true), nullptr);
        t.setProperty ("version", JucePlugin_VersionString, nullptr);
        writeExtraState (t);
        if (auto x = t.createXml()) copyXmlToBinary (*x, dest);
    }
    void setStateInformation (const void* data, int size) override
    {
        auto x = getXmlFromBinary (data, size); if (! x) return;
        auto t = juce::ValueTree::fromXml (*x); if (! t.hasType (state.state.getType())) return;
        if (auto* o = juce::JSON::parse (t.getProperty ("ui", "{}").toString()).getDynamicObject()) { const juce::ScopedLock sl (uiLock); ui = o->getProperties(); }
        readExtraState (t);
        for (auto* k : { "ui", "version" }) t.removeProperty (k, nullptr);
        state.replaceState (t);
    }
    virtual void writeExtraState (juce::ValueTree&) {}
    virtual void readExtraState (const juce::ValueTree&) {}

    float p (const char* id) const { return state.getRawParameterValue (id)->load (std::memory_order_relaxed); }

private:
    juce::CriticalSection uiLock;
    juce::NamedValueSet ui;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    Editor (Processor&, int faceW, int faceH);
    ~Editor() override;
    void resized() override { if (web) web->setBounds (getLocalBounds()); }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0b100e)); }

private:
    void timerCallback() override;
    juce::var stateForUi() const;
    std::optional<juce::WebBrowserComponent::Resource> serve (const juce::String& url);
    void applyZoom (int pct);

    Processor& proc;
    juce::SharedResourcePointer<Prefs> prefs;
    int faceW, faceH, tick = 0;
    std::map<juce::String, float> sent;
    std::set<juce::String> inGesture;
    std::unique_ptr<juce::WebBrowserComponent> web;
};

// ---- outils DSP partagés
struct Smooth
{
    void reset (double sr, double ms, float v) { k = (float) (1.0 - std::exp (-1.0 / (std::max (1e-3, ms * 0.001) * sr))); cur = v; }
    float next (float target) { cur += k * (target - cur); return cur; }
    float k = 1, cur = 0;
};
} // namespace rl
