#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/AfricanEngine.h"
#include "RLLicense.h"
#include <array>

class AfricanProcessor : public juce::AudioProcessor
{
public:
    AfricanProcessor();
    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override { return "African Riddim"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // un programme par instrument (54), dans l'ordre de l'index des planches
    int getNumPrograms() override { return 54; }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;            // notes jouées depuis l'interface
    afr::Feedback& feedback() { return engine.fb; }
    rl::License license;                        // Moonbase (réglages : licensing.json)
    juce::var getProjectUi() const;
    void setProjectUi (const juce::String& key, const juce::var& value);

private:
    afr::Engine engine;
    std::array<std::atomic<float>*, 9> rp {};
    int lastInstrument = -1;
    bool wasLicensed = true;
    juce::CriticalSection uiLock;
    juce::NamedValueSet projectUi;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AfricanProcessor)
};
