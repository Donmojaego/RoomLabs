#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/BassEngine.h"
#include <atomic>
#include <vector>

struct FactoryPreset
{
    juce::String name, section, rootName;
    int root = 29, outfit = 0;
    juce::NamedValueSet values;      // identifiant de paramètre -> valeur réelle
};

class ShakeItProcessor : public juce::AudioProcessor
{
public:
    ShakeItProcessor();
    ~ShakeItProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "Shake It"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return juce::jmax (1, (int) presets.size()); }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- utilisé par l'interface et les tests
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;            // notes jouées depuis le clavier de l'interface
    std::atomic<int> currentPreset { 0 };
    const std::vector<FactoryPreset>& factory() const { return presets; }
    void loadPreset (int index);                 // thread message
    shk::Motion& motion() { return engine.motion; }
    bool isSounding() const { return engine.isSounding(); }

    // état d'interface mémorisé dans le projet (tenue verrouillée, thème…) — sans effet sur le son
    juce::var getProjectUi() const;
    void setProjectUi (const juce::String& key, const juce::var& value);

private:
    void readParams (shk::Params&) const;
    void handleMidi (const juce::MidiMessage&);

    std::vector<FactoryPreset> presets;
    shk::BassEngine engine;

    struct Raw { std::atomic<float>* tune {}, * glide {}, * punch {}, * decay {}, * drive {}, * output {}, * harm {},
                 * tone {}, * reso {}, * attack {}, * release {}, * model {}, * glideMode {}, * driveType {}; } raw;

    juce::CriticalSection uiLock;
    juce::NamedValueSet projectUi;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShakeItProcessor)
};
