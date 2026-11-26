#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RLLicense.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/SoupEngine.h"
#include <array>
#include <atomic>
#include <vector>

struct FactoryPreset
{
    juce::String name, section, icon, rootName, mode { "soupe" };   // mode : soupe | ramen (banque asiatique / orientale)
    int sachet = 1;
    int root = 60;
    juce::NamedValueSet values;      // identifiant de paramètre -> valeur réelle
};

class SoupProcessor : public juce::AudioProcessor
{
public:
    rl::License license;                        // Moonbase (réglages : licensing.json)
    bool wasLicensed = true;
    std::atomic<float>* masterDb = nullptr; float lastMaster = 1.f;   // volume général
    SoupProcessor();
    ~SoupProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "Ma petite soupe ouselem"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return juce::jmax (1, (int) presets.size()); }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;            // notes jouées depuis l'interface (canal 1, percussions canal 10)
    std::atomic<int> currentPreset { 0 };
    std::atomic<double> hostBpm { 120.0 };
    const std::vector<FactoryPreset>& factory() const { return presets; }
    const juce::var& factoryInfo() const { return info; }   // sachets, instruments, effets
    void loadPreset (int index);                 // thread message
    soup::Feedback& feedback() { return engine.fb; }
    std::atomic<double> hostPpq { -1.0 };

    juce::var getProjectUi() const;
    void setProjectUi (const juce::String& key, const juce::var& value);

private:
    void readParams (soup::Params&) const;

    std::vector<FactoryPreset> presets;
    juce::var info;
    soup::SoupEngine engine;
    static constexpr std::array<const char*, 21> kIds { "inst", "fx", "division", "reverseOn", "stutterOn", "mix", "feedback", "stretch",
        "tone", "damp", "space", "level", "attack", "decay", "sustain", "release", "brightness", "tune", "arpOn", "arpMode", "arpOct" };
    std::array<std::atomic<float>*, 21> rp {};

    juce::CriticalSection uiLock;
    juce::NamedValueSet projectUi;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoupProcessor)
};
