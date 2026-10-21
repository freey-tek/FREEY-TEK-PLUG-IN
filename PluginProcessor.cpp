#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Algoritmos de distorsion (sin estado, se ejecutan a 4x sample rate)
//==============================================================================
namespace
{
    // Foldback: la senal que excede +-1 se "pliega" hacia dentro (onda triangular)
    inline float foldback (float x) noexcept
    {
        return std::abs (std::abs (std::fmod (x - 1.0f, 4.0f)) - 2.0f) - 1.0f;
    }

    // Valvula: tanh con bias (asimetria => armonicos pares), bias estatico compensado
    inline float tube (float x) noexcept
    {
        constexpr float bias = 0.25f;
        static const float offset = std::tanh (bias);
        return std::tanh (x + bias) - offset;
    }

    // Diodo: saturacion exponencial asimetrica (semiciclo negativo mas duro)
    inline float diode (float x) noexcept
    {
        if (x >= 0.0f)
            return 1.0f - std::exp (-x);

        return -0.85f * (1.0f - std::exp (1.8f * x));
    }

    template <typename Fn>
    void shapeBlock (juce::dsp::AudioBlock<float>& block, Fn&& fn)
    {
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* d = block.getChannelPointer (ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
                d[i] = fn (d[i]);
        }
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout FreeyTekAudioProcessor::createParameterLayout()
{
    using namespace juce;
    using Range = NormalisableRange<float>;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // --- COMP ---
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                    Range (-60.0f, 0.0f, 0.01f), -20.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));

    {
        Range r (1.0f, 20.0f, 0.01f);
        r.setSkewForCentre (4.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::ratio, 1 }, "Ratio",
                        r, 4.0f, AudioParameterFloatAttributes().withLabel (":1")));
    }
    {
        Range r (0.1f, 50.0f, 0.01f);
        r.setSkewForCentre (5.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                        r, 5.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    }
    {
        Range r (10.0f, 1000.0f, 0.1f);
        r.setSkewForCentre (100.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::release, 1 }, "Release",
                        r, 100.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    }

    // --- DIST ---
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::drive, 1 }, "Drive",
                    Range (1.0f, 10.0f, 0.01f), 1.0f));
    {
        Range r (200.0f, 20000.0f, 1.0f);
        r.setSkewForCentre (2000.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::tone, 1 }, "Tone",
                        r, 8000.0f, AudioParameterFloatAttributes().withLabel ("Hz")));
    }
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::distType, 1 }, "Type",
                    StringArray { "Foldback", "Valve/Tube", "Diode/Saturator" }, 0));

    // --- MASTER ---
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::mix, 1 }, "Dry/Wet",
                    Range (0.0f, 100.0f, 0.1f), 100.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                    Range (-60.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));

    return { p.begin(), p.end() };
}

//==============================================================================
FreeyTekAudioProcessor::FreeyTekAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FREEYTEK_STATE", createParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue (ParamIDs::threshold);
    ratioParam     = apvts.getRawParameterValue (ParamIDs::ratio);
    attackParam    = apvts.getRawParameterValue (ParamIDs::attack);
    releaseParam   = apvts.getRawParameterValue (ParamIDs::release);
    driveParam     = apvts.getRawParameterValue (ParamIDs::drive);
    toneParam      = apvts.getRawParameterValue (ParamIDs::tone);
    typeParam      = apvts.getRawParameterValue (ParamIDs::distType);
    mixParam       = apvts.getRawParameterValue (ParamIDs::mix);
    outputParam    = apvts.getRawParameterValue (ParamIDs::output);
}

//==============================================================================
bool FreeyTekAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

//==============================================================================
void FreeyTekAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();

    const int latency = (int) std::round (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();

    dryBuffer.setSize (2, samplesPerBlock);

    for (auto* f : { &dcBlock, &toneLP1, &toneLP2 })
    {
        f->prepare (spec);
        f->reset();
    }
    dcBlock.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    dcBlock.setCutoffFrequency (20.0f);
    toneLP1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    toneLP2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    const double ramp = 0.03;
    driveSmoothed.reset (sampleRate, ramp);
    mixSmoothed.reset   (sampleRate, ramp);
    outSmoothed.reset   (sampleRate, ramp);
    toneSmoothed.reset  (sampleRate, 0.05);

    driveSmoothed.setCurrentAndTargetValue (*driveParam);
    mixSmoothed.setCurrentAndTargetValue   (*mixParam * 0.01f);
    outSmoothed.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain ((float) *outputParam, -60.0f));
    toneSmoothed.setCurrentAndTargetValue  (*toneParam);

    grDb = 0.0f;
}

//==============================================================================
void FreeyTekAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh == 0 || numSamples == 0)
        return;

    //--------------------------------------------------------------------------
    // 0. Copia de la senal seca (retrasada para igualar la latencia del oversampling)
    dryBuffer.setSize (numCh, numSamples, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* in  = buffer.getReadPointer (ch);
        auto*       dry = dryBuffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
        {
            dryDelay.pushSample (ch, in[n]);
            dry[n] = dryDelay.popSample (ch);
        }
    }

    //--------------------------------------------------------------------------
    // 1. COMPRESOR (feed-forward, detector de picos enlazado, rodilla de 3 dB)
    {
        const float sr          = (float) currentSampleRate;
        const float thresholdDb = thresholdParam->load();
        const float ratio       = juce::jmax (1.0f, ratioParam->load());
        const float attackMs    = juce::jmax (0.01f, attackParam->load());
        const float releaseMs   = juce::jmax (1.0f,  releaseParam->load());

        const float atkCoeff = std::exp (-1.0f / (0.001f * attackMs  * sr));
        const float relCoeff = std::exp (-1.0f / (0.001f * releaseMs * sr));
        const float slope    = 1.0f / ratio - 1.0f;          // <= 0
        constexpr float kneeDb = 3.0f;

        // Makeup automatico parcial (50 %) para mantener el punch
        const float makeupDb = 0.5f * (-thresholdDb) * (1.0f - 1.0f / ratio);

        for (int n = 0; n < numSamples; ++n)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[n]));

            const float levelDb = juce::Decibels::gainToDecibels (peak, -120.0f);
            const float over    = levelDb - thresholdDb;

            float grTarget;
            if (2.0f * over < -kneeDb)
                grTarget = 0.0f;
            else if (2.0f * std::abs (over) <= kneeDb)
            {
                const float t = over + kneeDb * 0.5f;
                grTarget = slope * t * t / (2.0f * kneeDb);
            }
            else
                grTarget = slope * over;

            const float c = (grTarget < grDb) ? atkCoeff : relCoeff;
            grDb = grTarget + c * (grDb - grTarget);

            const float gain = juce::Decibels::decibelsToGain (grDb + makeupDb);

            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[n] *= gain;
        }
    }

    //--------------------------------------------------------------------------
    // 2. DISTORSION
    {
        driveSmoothed.setTargetValue (driveParam->load());

        // Ganancia de drive (1..10 => 1x..100x, hasta +40 dB)
        for (int n = 0; n < numSamples; ++n)
        {
            const float d = driveSmoothed.getNextValue();
            const float g = d * d;
            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[n] *= g;
        }

        juce::dsp::AudioBlock<float> block (buffer);
        auto osBlock = oversampling.processSamplesUp (block);

        switch ((int) typeParam->load())
        {
            case 0:  shapeBlock (osBlock, [] (float x) { return foldback (x); }); break;
            case 1:  shapeBlock (osBlock, [] (float x) { return tube (x);     }); break;
            default: shapeBlock (osBlock, [] (float x) { return diode (x);    }); break;
        }

        oversampling.processSamplesDown (block);

        // Trim de headroom
        buffer.applyGain (0.8f);

        // DC blocker + Tone (low-pass de 24 dB/oct)
        toneSmoothed.setTargetValue (toneParam->load());
        const float cutoff = juce::jmin (toneSmoothed.skip (numSamples),
                                         0.45f * (float) currentSampleRate);
        toneLP1.setCutoffFrequency (cutoff);
        toneLP2.setCutoffFrequency (cutoff);

        juce::dsp::ProcessContextReplacing<float> ctx (block);
        dcBlock.process (ctx);
        toneLP1.process (ctx);
        toneLP2.process (ctx);
    }

    //--------------------------------------------------------------------------
    // 3. MIX DRY/WET + 4. OUTPUT GAIN
    mixSmoothed.setTargetValue (mixParam->load() * 0.01f);
    outSmoothed.setTargetValue (juce::Decibels::decibelsToGain ((float) outputParam->load(), -60.0f));

    for (int n = 0; n < numSamples; ++n)
    {
        const float m  = mixSmoothed.getNextValue();
        const float og = outSmoothed.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            const float dry = dryBuffer.getReadPointer (ch)[n];
            wet[n] = (dry * (1.0f - m) + wet[n] * m) * og;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* FreeyTekAudioProcessor::createEditor()
{
    return new FreeyTekAudioProcessorEditor (*this);
}

//==============================================================================
void FreeyTekAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FreeyTekAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreeyTekAudioProcessor();
}
