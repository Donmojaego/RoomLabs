#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/SmokeEngine.h"
#include <array>
#include <atomic>
#include <vector>

struct FactoryPreset
{
    juce::String name, section, rootName;
    int bank = 0;
    int root = 60;
    juce::NamedValueSet values;      // identifiant de paramètre -> valeur réelle
};

class SmokeProcessor : public juce::AudioProcessor
{
public:
    SmokeProcessor();
    ~SmokeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "Smoke"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return juce::jmax (1, (int) presets.size()); }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;            // notes jouées depuis l'interface
    std::atomic<int> currentPreset { 0 };
    const std::vector<FactoryPreset>& factory() const { return presets; }
    const juce::var& factoryInfo() const { return info; }   // banques et presets
    void loadPreset (int index);                 // thread message
    smk::Feedback& feedback() { return engine.fb; }

    juce::var getProjectUi() const;
    void setProjectUi (const juce::String& key, const juce::var& value);

private:
    void readParams (smk::Params&) const;

    std::vector<FactoryPreset> presets;
    juce::var info;
    smk::SmokeEngine engine;
    static constexpr std::array<const char*, 17> kIds { "model", "tone", "space", "mod", "dream", "clean", "level", "attack", "decay",
        "sustain", "release", "bright", "tremolo", "rise", "glide", "chord", "tune" };
    std::array<std::atomic<float>*, 17> rp {};

    juce::CriticalSection uiLock;
    juce::NamedValueSet projectUi;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmokeProcessor)
};
