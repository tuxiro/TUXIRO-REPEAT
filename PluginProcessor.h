#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "RiserEngine.h"

class TuxiroProcessor : public juce::AudioProcessor
{
public:
    TuxiroProcessor();
    ~TuxiroProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TUXIRO REPEAT"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- utilidades para la interfaz ----
    void setParamPlain (const juce::String& id, float plainValue);
    void applyPreset (int index);
    void randomize();
    void snapshotUndo();
    void undo();
    trx::RiserParams readParams() const;
    bool renderToWav (const juce::File& dest);   // render offline del riser actual

    static const juce::StringArray presetNames;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<int>   uiCommand { 0 };     // 1 = play (preview), 2 = stop
    std::atomic<int>   uiState { 0 };       // 0 idle, 1 building, 2 gap/drop
    std::atomic<float> uiFraction { 0.0f };
    std::atomic<float> uiBpm { 128.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    trx::RiserEngine engine;
    bool previewing = false;
    int activeNote = -1;
    juce::ValueTree undoState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuxiroProcessor)
};
