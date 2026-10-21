#pragma once

#include "PluginProcessor.h"

// ---------------------------------------------------------------------------
// Opcional: si quieres usar TU PNG como fondo (arte completo), exporta la
// imagen SIN perillas ni textos de controles, anadela al proyecto como recurso
// binario con el nombre "background.png" y define FREEYTEK_USE_BACKGROUND_IMAGE=1
// (Preprocessor Definitions). Las perillas se colocan sobre el arte en las
// mismas posiciones que en el diseno original.
// ---------------------------------------------------------------------------
#ifndef FREEYTEK_USE_BACKGROUND_IMAGE
 #define FREEYTEK_USE_BACKGROUND_IMAGE 0
#endif

//==============================================================================
class FreeyTekLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FreeyTekLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

//==============================================================================
class FreeyTekAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FreeyTekAudioProcessorEditor (FreeyTekAudioProcessor&);
    ~FreeyTekAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void initKnob (juce::Slider&, std::unique_ptr<SliderAttachment>&, const char* paramID);
    void drawProceduralArt (juce::Graphics&, float s);
    void buildTexture();

    FreeyTekAudioProcessor& processor;
    FreeyTekLookAndFeel lnf;

    // COMP
    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider;
    // DIST
    juce::Slider driveSlider, toneSlider, typeSlider;
    // MASTER
    juce::Slider mixSlider, outputSlider;

    // Los attachments se declaran DESPUES de los sliders (se destruyen antes)
    std::unique_ptr<SliderAttachment> thresholdAtt, ratioAtt, attackAtt, releaseAtt,
                                      driveAtt, toneAtt, typeAtt, mixAtt, outputAtt;

    juce::Image texture;      // grano + rayones generados proceduralmente
    juce::Image artImage;     // fondo opcional (BinaryData)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreeyTekAudioProcessorEditor)
};
