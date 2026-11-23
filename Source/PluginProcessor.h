#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
#include "Fx.h"
#include "Presets.h"

class JustAnAmpProcessor : public juce::AudioProcessor
{
public:
    JustAnAmpProcessor();
    ~JustAnAmpProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JustAnAmp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 7.0; }

    int getNumPrograms() override { return jah::presetCount(); }
    int getCurrentProgram() override { return juce::jlimit (0, jah::presetCount() - 1, (int) apvts.state.getProperty ("presetIndex", 0)); }
    void setCurrentProgram (int i) override { if (i != getCurrentProgram()) loadPreset (i); }
    const juce::String getProgramName (int i) override { return jah::presetName (i); }
    void loadPreset (int i)
    {
        jah::applyPreset (apvts, i);
        apvts.state.setProperty ("presetIndex", i, nullptr);
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    jah::Engine engine;
    jah::FxChain fx;
    jah::NoiseGate gate;
    jah::Compressor comp;
    std::vector<float> mono;
    juce::SmoothedValue<float> master;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustAnAmpProcessor)
};
