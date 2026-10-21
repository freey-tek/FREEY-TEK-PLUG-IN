#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
namespace ParamIDs
{
    inline constexpr const char* threshold = "threshold";
    inline constexpr const char* ratio     = "ratio";
    inline constexpr const char* attack    = "attack";
    inline constexpr const char* release   = "release";
    inline constexpr const char* drive     = "drive";
    inline constexpr const char* tone      = "tone";
    inline constexpr const char* distType  = "dist_type";
    inline constexpr const char* mix       = "mix";
    inline constexpr const char* output    = "output";
}

//==============================================================================
class FreeyTekAudioProcessor : public juce::AudioProcessor
{
public:
    FreeyTekAudioProcessor();
    ~FreeyTekAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FREEY-TEK COMP-DIST"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    //==========================================================================
    // Parametros (punteros atomicos cacheados)
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* driveParam     = nullptr;
    std::atomic<float>* toneParam      = nullptr;
    std::atomic<float>* typeParam      = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* outputParam    = nullptr;

    //==========================================================================
    // Compresor
    float grDb = 0.0f;   // reduccion de ganancia suavizada (dB, <= 0), enlazada en estereo

    // Distorsion (4x oversampling para reducir aliasing)
    juce::dsp::Oversampling<float> oversampling { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };

    juce::dsp::StateVariableTPTFilter<float> dcBlock, toneLP1, toneLP2;

    // Dry path con compensacion de latencia
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuffer;

    // Smoothers
    juce::SmoothedValue<float> driveSmoothed, mixSmoothed, outSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> toneSmoothed;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreeyTekAudioProcessor)
};
