#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <map>
#include <optional>
#include <set>

// Préférences d'interface (thème, taille, mode visuel, tenue) : communes à tous
// les projets, séparées du son, des presets et de l'automation.
struct UiPrefs
{
    UiPrefs();
    juce::PropertiesFile& file() { return *props; }
    juce::var toVar() const;
private:
    std::unique_ptr<juce::PropertiesFile> props;
};

class SmokeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SmokeEditor (SmokeProcessor&);
    ~SmokeEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff120a16)); }
    void visibilityChanged() override;

    static constexpr int baseW = 1000, baseH = 640;

private:
    void timerCallback() override;
    juce::var stateForUi (bool full) const;
    juce::var mergedPrefs() const;
    void setParamFromUi (const juce::String& id, float value);
    void applyZoom (int percent);
    std::optional<juce::WebBrowserComponent::Resource> serve (const juce::String& url);

    SmokeProcessor& proc;
    juce::SharedResourcePointer<UiPrefs> prefs;
    std::map<juce::String, float> sent;          // dernières valeurs envoyées à l'interface
    std::set<juce::String> inGesture;
    int tick = 0, idleTicks = 0, lastOnsets = -1, lastPreset = -1;
    std::unique_ptr<juce::WebBrowserComponent> web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmokeEditor)
};
