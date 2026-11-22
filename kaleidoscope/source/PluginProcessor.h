#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/Generator.h"
#include "engine/SynthEngine.h"
#include <array>
#include <atomic>
#include <deque>
#include <memory>
#include <vector>

// Preset : 150 natifs (synthèse) + 100 fonds (instruments échantillonnés installés à part)
struct FactoryPreset
{
    juce::String id, name, cat, bank, version; int style = -1, mood = -1; bool fond = false;
    juce::NamedValueSet values;
};

// Motif prêt à jouer (temps en noires), échangé avec le fil audio par pointeur atomique
struct Playable
{
    struct Ev { double start, len; int pitch; float vel; };
    std::vector<Ev> notes; double lengthBeats = 16; uint64_t serial = 0;
};

// Banques de fonds : dossiers « Banques/<id>/instrument.json » (installeur des fonds)
class BankLibrary
{
public:
    BankLibrary();
    juce::Array<juce::File> roots() const;
    juce::File folderFor (const juce::String& id) const;          // vide si non installé
    bool installed (const juce::String& id) const { return folderFor (id).isDirectory(); }
    std::unique_ptr<kal::SamplePatch> load (const juce::String& id, juce::String& error) const;
private:
    juce::AudioFormatManager formats;
};

class KaleidoProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    KaleidoProcessor();
    ~KaleidoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override { return "Kaleidoscope"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return juce::jmax (1, (int) presets.size()); }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- thread message (interface)
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;
    std::atomic<int> currentPreset { 0 };
    const std::vector<FactoryPreset>& factory() const { return presets; }
    void loadPreset (int index);
    juce::var generate();                       // Générer : nouvelle proposition
    juce::var decline (bool fromCurrent = false); // Déclinaison de la référence (1re mélodie, ou verrouillée)
    juce::var lockReference();                  // la proposition courante devient la référence
    juce::var previous();                       // revenir à la proposition précédente
    juce::var feedback (const juce::String& kind);   // like | keep | redo
    void resetPersonal();
    juce::var setDrop (const juce::var& analysis);   // { root, scale, bpm, confidence }
    juce::File exportMidi (juce::File dest = {});    // .mid de la proposition courante
    juce::var patternInfo() const;               // notes + traits (kaléidoscope, mini-rouleau)
    juce::var bankInfo() const;                  // fonds installés + fond chargé
    juce::String loadedBank() const { const juce::ScopedLock sl (bankLock); return bankId; }
    juce::String bankError() const { const juce::ScopedLock sl (bankLock); return bankErr; }
    juce::var getProjectUi() const;
    void setProjectUi (const juce::String& key, const juce::var& value);
    void pumpForTests() { timerCallback(); }

    // ---- retour vers l'interface
    kal::Feedback& feedback() { return engine.fb; }
    std::atomic<double> hostBpm { 110.0 }, beatPos { 0.0 };
    std::atomic<int> loopsDone { 0 }, seqStep { -1 };
    std::atomic<uint64_t> playingSerial { 0 };
    std::atomic<int> patternSerial { 0 };

private:
    void timerCallback() override;
    kal::Request makeRequest() const;
    void install (const kal::Pattern& p, bool remember);
    void requestBank (const juce::String& id);
    void readSound (kal::SynthParams&) const;
    int choiceIndex (const char* id) const;
    float value (const char* id) const;

    std::vector<FactoryPreset> presets;
    kal::SynthEngine engine;
    kal::Generator gen;
    kal::Profile profile;
    kal::Pattern current; bool hasPattern = false;
    kal::Pattern reference; bool hasRef = false;
    std::deque<kal::Pattern> past;              // propositions précédentes (bouton Précédent)
    juce::File profileFile() const;

    // motif lu par le fil audio
    std::atomic<const Playable*> live { nullptr };
    std::vector<std::pair<std::unique_ptr<Playable>, double>> retired;   // gardés vivants quelques secondes
    std::unique_ptr<Playable> owned;
    uint64_t serialCounter = 0;

    // séquenceur (fil audio)
    double freeBeat = 0, lastEnd = -1; bool wasPlaying = false; int lastLoop = -1;
    struct Off { double beat; int pitch; };
    std::array<Off, 64> offs {}; int nOffs = 0;
    uint32_t humRng = 0x2545F491u;
    int loopsSeen = 0;

    // fonds (chargement en arrière-plan)
    BankLibrary banks;
    juce::ThreadPool loader { 1 };
    mutable juce::CriticalSection bankLock;
    juce::String bankId, bankErr, wantedBank;
    std::unique_ptr<kal::SamplePatch> patch;
    std::vector<std::pair<std::unique_ptr<kal::SamplePatch>, double>> oldPatches;
    std::atomic<int> bankJobs { 0 };

    // paramètres lus en temps réel
    std::vector<std::pair<juce::String, std::atomic<float>*>> soundRaw;
    std::atomic<float>* pPlay = nullptr, *pFollow = nullptr, *pTempo = nullptr, *pHuman = nullptr, *pGate = nullptr, *pMidiOut = nullptr, *pEvolve = nullptr;

    juce::CriticalSection uiLock;
    juce::NamedValueSet projectUi;
    int evolveCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KaleidoProcessor)
};
