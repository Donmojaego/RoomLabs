#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <map>
#include <optional>
#include <set>

// Préférences d'interface (layout, taille, animation) : communes à tous les projets, séparées du son.
struct UiPrefs
{
    UiPrefs();
    juce::PropertiesFile& file() { return *props; }
    juce::var toVar() const;
private:
    std::unique_ptr<juce::PropertiesFile> props;
};

class KaleidoEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit KaleidoEditor (KaleidoProcessor&);
    ~KaleidoEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff07060c)); }
    void visibilityChanged() override;
    static constexpr int baseW = 1000, baseH = 1083;

private:
    void timerCallback() override;
    juce::var stateForUi (bool full) const;
    juce::var mergedPrefs() const;
    void setParamFromUi (const juce::String& id, float value);
    void applyZoom (int percent);
    std::optional<juce::WebBrowserComponent::Resource> serve (const juce::String& url);
    juce::File assetsOverride() const;           // dossier d'assets de layout posé à côté (développement)

    KaleidoProcessor& proc;
    juce::SharedResourcePointer<UiPrefs> prefs;
    std::map<juce::String, float> sent;
    std::set<juce::String> inGesture;
    int tick = 0, idleTicks = 0, lastHits = -1, lastPreset = -1, lastPattern = -1, lastStep = -2;
    juce::String lastBankState;
    std::unique_ptr<juce::WebBrowserComponent> web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KaleidoEditor)
};
