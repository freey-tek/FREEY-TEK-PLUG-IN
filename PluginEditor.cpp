#include "PluginEditor.h"

#if FREEYTEK_USE_BACKGROUND_IMAGE
 #include "BinaryData.h"   // generado por Projucer/CMake (juce_add_binary_data)
#endif

//==============================================================================
// Paleta tomada del diseno
//==============================================================================
namespace Pal
{
    const juce::Colour bg      { 0xff0b0a0a };
    const juce::Colour bone    { 0xffe6d9c3 };   // texto / lineas hueso
    const juce::Colour boneDim { 0xff9c9382 };
    const juce::Colour blood   { 0xffc0201f };   // acentos rojos
    const juce::Colour bloodDk { 0xff4a0f0f };
    const juce::Colour metalHi { 0xff7a756c };
    const juce::Colour metalLo { 0xff131212 };
}

namespace
{
    constexpr float refW = 1664.0f;   // tamano de referencia de la imagen de diseno
    constexpr float refH = 945.0f;

    juce::Font monoFont (float height, bool bold = false)
    {
       #if JUCE_MAJOR_VERSION >= 8
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height,
                                              bold ? juce::Font::bold : juce::Font::plain));
       #else
        return juce::Font (juce::Font::getDefaultMonospacedFontName(), height,
                           bold ? juce::Font::bold : juce::Font::plain);
       #endif
    }

    juce::String xi()      { return juce::String::charToString ((juce::juce_wchar) 0x039E); } // Ξ
    juce::String oSlash()  { return juce::String::charToString ((juce::juce_wchar) 0x00D8); } // Ø

    void strokePentagram (juce::Graphics& g, juce::Point<float> c, float r, float thickness)
    {
        juce::Path p;
        for (int i = 0; i < 5; ++i)
        {
            const float a = -juce::MathConstants<float>::halfPi + (float) i * 4.0f * juce::MathConstants<float>::pi / 5.0f;
            const juce::Point<float> pt (c.x + r * std::cos (a), c.y + r * std::sin (a));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        g.strokePath (p, juce::PathStrokeType (thickness));
        g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, thickness * 0.7f);
    }

    void strokeEyeTriangle (juce::Graphics& g, juce::Point<float> c, float w, float h,
                            float thickness, juce::Colour line, juce::Colour pupil)
    {
        g.setColour (line);
        juce::Path tri;
        tri.startNewSubPath (c.x, c.y - h * 0.55f);
        tri.lineTo (c.x - w * 0.5f, c.y + h * 0.45f);
        tri.lineTo (c.x + w * 0.5f, c.y + h * 0.45f);
        tri.closeSubPath();
        g.strokePath (tri, juce::PathStrokeType (thickness));

        // rayos
        for (int i = -5; i <= 5; ++i)
        {
            const float a = -juce::MathConstants<float>::halfPi + (float) i * 0.16f;
            g.drawLine (c.x + std::cos (a) * h * 0.62f, c.y + h * 0.05f + std::sin (a) * h * 0.62f,
                        c.x + std::cos (a) * h * 0.86f, c.y + h * 0.05f + std::sin (a) * h * 0.86f,
                        thickness * 0.5f);
        }

        // ojo
        const float ew = w * 0.22f, eh = h * 0.14f;
        juce::Path eye;
        eye.startNewSubPath (c.x - ew, c.y + h * 0.12f);
        eye.quadraticTo (c.x, c.y + h * 0.12f - eh * 2.0f, c.x + ew, c.y + h * 0.12f);
        eye.quadraticTo (c.x, c.y + h * 0.12f + eh * 2.0f, c.x - ew, c.y + h * 0.12f);
        g.strokePath (eye, juce::PathStrokeType (thickness * 0.8f));

        g.setColour (pupil);
        g.fillEllipse (c.x - eh * 0.6f, c.y + h * 0.12f - eh * 0.6f, eh * 1.2f, eh * 1.2f);
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float chamfer, float s)
    {
        auto makePath = [chamfer] (juce::Rectangle<float> b)
        {
            juce::Path p;
            p.startNewSubPath (b.getX() + chamfer, b.getY());
            p.lineTo (b.getRight() - chamfer, b.getY());
            p.lineTo (b.getRight(), b.getY() + chamfer);
            p.lineTo (b.getRight(), b.getBottom() - chamfer);
            p.lineTo (b.getRight() - chamfer, b.getBottom());
            p.lineTo (b.getX() + chamfer, b.getBottom());
            p.lineTo (b.getX(), b.getBottom() - chamfer);
            p.lineTo (b.getX(), b.getY() + chamfer);
            p.closeSubPath();
            return p;
        };

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillPath (makePath (r));

        g.setColour (Pal::bone.withAlpha (0.65f));
        g.strokePath (makePath (r), juce::PathStrokeType (2.0f * s));

        g.setColour (Pal::blood.withAlpha (0.40f));
        g.strokePath (makePath (r.reduced (9.0f * s)), juce::PathStrokeType (1.0f * s));
    }
}

//==============================================================================
// LookAndFeel: perillas de metal oscuro con indicador rojo
//==============================================================================
FreeyTekLookAndFeel::FreeyTekLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    Pal::blood);
    setColour (juce::Slider::rotarySliderOutlineColourId, Pal::bloodDk);
    setColour (juce::TooltipWindow::backgroundColourId,   Pal::bg);
    setColour (juce::TooltipWindow::textColourId,         Pal::bone);
    setColour (juce::TooltipWindow::outlineColourId,      Pal::blood);
    setColour (juce::BubbleComponent::backgroundColourId, Pal::bg);
    setColour (juce::BubbleComponent::outlineColourId,    Pal::blood);
}

void FreeyTekLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float sliderPos, float startAngle, float endAngle,
                                            juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c      = bounds.getCentre();
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    // Pista + arco de valor
    const float trackR = radius * 0.95f;
    const float thick  = juce::jmax (2.0f, radius * 0.09f);

    juce::Path track, value;
    track.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, startAngle, angle, true);

    g.setColour (Pal::bloodDk);
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Pal::blood);
    g.strokePath (value, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Cuerpo metalico
    const float kr = radius * 0.72f;
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - kr - 2.0f, c.y - kr - 2.0f, 2 * kr + 4.0f, 2 * kr + 4.0f);

    juce::ColourGradient body (Pal::metalHi, c.x - kr * 0.4f, c.y - kr * 0.6f,
                               Pal::metalLo, c.x + kr * 0.7f, c.y + kr * 0.9f, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - kr, c.y - kr, 2 * kr, 2 * kr);

    // Bisel interior
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (c.x - kr * 0.72f, c.y - kr * 0.72f, kr * 1.44f, kr * 1.44f, 1.2f);
    g.setColour (Pal::bone.withAlpha (0.18f));
    g.drawEllipse (c.x - kr, c.y - kr, 2 * kr, 2 * kr, 1.0f);

    // Indicador rojo
    juce::Path pointer;
    const float inner = kr * 0.18f, outer = kr * 0.92f;
    pointer.startNewSubPath (c.x + inner * std::sin (angle), c.y - inner * std::cos (angle));
    pointer.lineTo          (c.x + outer * std::sin (angle), c.y - outer * std::cos (angle));
    g.setColour (Pal::blood);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.0f, radius * 0.08f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

//==============================================================================
FreeyTekAudioProcessorEditor::FreeyTekAudioProcessorEditor (FreeyTekAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
   #if FREEYTEK_USE_BACKGROUND_IMAGE
    artImage = juce::ImageCache::getFromMemory (BinaryData::background_png,
                                                BinaryData::background_pngSize);
   #endif

    initKnob (thresholdSlider, thresholdAtt, ParamIDs::threshold);
    initKnob (ratioSlider,     ratioAtt,     ParamIDs::ratio);
    initKnob (attackSlider,    attackAtt,    ParamIDs::attack);
    initKnob (releaseSlider,   releaseAtt,   ParamIDs::release);
    initKnob (driveSlider,     driveAtt,     ParamIDs::drive);
    initKnob (toneSlider,      toneAtt,      ParamIDs::tone);
    initKnob (typeSlider,      typeAtt,      ParamIDs::distType);
    initKnob (mixSlider,       mixAtt,       ParamIDs::mix);
    initKnob (outputSlider,    outputAtt,    ParamIDs::output);

    typeSlider.onValueChange = [this] { repaint(); };   // refresca el nombre del algoritmo

    setResizable (true, true);
    setResizeLimits (832, 472, 1664, 945);
    getConstrainer()->setFixedAspectRatio ((double) refW / (double) refH);
    setSize (1040, 590);
}

FreeyTekAudioProcessorEditor::~FreeyTekAudioProcessorEditor()
{
    for (auto* s : { &thresholdSlider, &ratioSlider, &attackSlider, &releaseSlider,
                     &driveSlider, &toneSlider, &typeSlider, &mixSlider, &outputSlider })
        s->setLookAndFeel (nullptr);
}

void FreeyTekAudioProcessorEditor::initKnob (juce::Slider& s, std::unique_ptr<SliderAttachment>& att,
                                             const char* paramID)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setPopupDisplayEnabled (true, true, this);   // valor mientras arrastras
    s.setLookAndFeel (&lnf);
    addAndMakeVisible (s);

    att = std::make_unique<SliderAttachment> (processor.apvts, paramID, s);
}

//==============================================================================
void FreeyTekAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / refW;

    auto place = [s] (juce::Slider& sl, float cx, float cy, float d)
    {
        sl.setBounds (juce::Rectangle<float> (d * s, d * s)
                          .withCentre ({ cx * s, cy * s }).toNearestInt());
    };

    // Posiciones (centro de cada perilla) tomadas del diseno de 1664x945
    place (thresholdSlider, 390.0f, 342.0f, 88.0f);
    place (ratioSlider,     390.0f, 424.0f, 88.0f);
    place (attackSlider,    390.0f, 509.0f, 88.0f);
    place (releaseSlider,   390.0f, 595.0f, 88.0f);

    place (driveSlider,    1028.0f, 342.0f, 88.0f);
    place (toneSlider,     1028.0f, 428.0f, 88.0f);
    place (typeSlider,     1028.0f, 514.0f, 88.0f);

    place (mixSlider,       365.0f, 771.0f, 84.0f);
    place (outputSlider,    940.0f, 771.0f, 84.0f);

    buildTexture();
}

//==============================================================================
void FreeyTekAudioProcessorEditor::buildTexture()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;

    texture = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (texture);
    juce::Random rng (1337);

    // Grano
    for (int i = 0; i < (w * h) / 55; ++i)
    {
        g.setColour (juce::Colour::fromFloatRGBA (0.9f, 0.85f, 0.75f, rng.nextFloat() * 0.07f));
        g.fillRect (rng.nextInt (w), rng.nextInt (h), 1, 1);
    }

    // Rayones hueso
    for (int i = 0; i < 260; ++i)
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float len = 8.0f + rng.nextFloat() * 70.0f;
        g.setColour (Pal::bone.withAlpha (0.03f + rng.nextFloat() * 0.09f));
        g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.7f);
    }

    // Venas rojas
    for (int i = 0; i < 40; ++i)
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float len = 20.0f + rng.nextFloat() * 120.0f;
        g.setColour (Pal::blood.withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.8f);
    }
}

//==============================================================================
void FreeyTekAudioProcessorEditor::drawProceduralArt (juce::Graphics& g, float s)
{
    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * s, y * s, w * s, h * s);
    };

    // ---- Marco exterior ----
    g.setColour (Pal::bone.withAlpha (0.55f));
    g.drawRect (R (8, 8, refW - 16, refH - 16), 2.0f * s);
    g.setColour (Pal::blood.withAlpha (0.45f));
    g.drawRect (R (18, 18, refW - 36, refH - 36), 1.0f * s);

    // ---- Titulo ----
    const auto titleL = "FR" + xi() + xi() + "Y-T" + xi() + "K";
    const juce::String titleR = "COMP-DIST";

    g.setFont (monoFont (84.0f * s, true));
    g.setColour (Pal::blood.withAlpha (0.85f));
    g.drawText (titleL, R (253, 42, 545, 115), juce::Justification::centred);
    g.drawText (titleR, R (883, 42, 480, 115), juce::Justification::centred);
    g.setColour (Pal::bone);
    g.drawText (titleL, R (250, 40, 545, 115), juce::Justification::centred);
    g.drawText (titleR, R (880, 40, 480, 115), juce::Justification::centred);

    // Cruz invertida central del titulo
    g.setColour (Pal::bone);
    g.drawLine (835 * s, 55 * s, 835 * s, 150 * s, 3.0f * s);
    g.drawLine (815 * s, 85 * s, 855 * s, 85 * s, 3.0f * s);
    g.drawLine (822 * s, 120 * s, 848 * s, 120 * s, 3.0f * s);

    // Lema
    g.setFont (monoFont (24.0f * s).withExtraKerningFactor (0.35f));
    g.setColour (Pal::blood);
    g.drawText ("TODO TIENE UN PRECIO", R (500, 155, 660, 34), juce::Justification::centred);

    // Caja VST3
    g.setColour (Pal::bone.withAlpha (0.7f));
    g.drawRect (R (1365, 68, 230, 87), 2.0f * s);
    g.setFont (monoFont (54.0f * s, true));
    g.setColour (Pal::bone);
    g.drawText ("[ VST3 ]", R (1365, 68, 230, 87), juce::Justification::centred);

    // ---- Paneles ----
    drawPanel (g, R (245, 200, 490, 455), 22.0f * s, s);   // COMP
    drawPanel (g, R (925, 215, 490, 440), 22.0f * s, s);   // DIST
    drawPanel (g, R (225, 668, 1255, 160), 22.0f * s, s);  // MASTER

    g.setFont (monoFont (46.0f * s));
    g.setColour (Pal::bone);
    g.drawText ("[COMP]",   R (298, 235, 300, 50), juce::Justification::centredLeft);
    g.drawText ("[DIST]",   R (960, 235, 300, 50), juce::Justification::centredLeft);
    g.drawText ("[MASTER]", R (280, 678, 400, 50), juce::Justification::centredLeft);

    // ---- Etiquetas de controles ----
    g.setFont (monoFont (36.0f * s));
    auto label = [&] (const juce::String& t, float x, float cy)
    {
        g.drawText (t, R (x, cy - 26.0f, 330, 52), juce::Justification::centredLeft);
    };
    label ("Threshold", 482, 342);
    label ("Ratio",     485, 424);
    label ("Attack",    485, 509);
    label ("Release",   485, 595);
    label ("Drive",    1128, 342);
    label ("Tone",     1128, 428);
    label ("Type",     1128, 514);
    label ("Dry/Wet",   443, 771);
    label ("Output",   1010, 771);

    g.setFont (monoFont (22.0f * s));
    g.setColour (Pal::boneDim);
    g.drawText ("(Foldback / Tube / Diode)", R (1095, 550, 330, 30), juce::Justification::centredLeft);

    // ---- Columna central: ojo, serpiente y eje ----
    strokeEyeTriangle (g, { 830 * s, 248 * s }, 125 * s, 110 * s, 2.0f * s, Pal::bone, Pal::blood);

    g.setColour (Pal::bone.withAlpha (0.85f));
    g.drawLine (830 * s, 305 * s, 830 * s, 790 * s, 2.5f * s);
    g.drawLine (800 * s, 330 * s, 860 * s, 330 * s, 2.0f * s);

    for (int phase = 0; phase < 2; ++phase)
    {
        juce::Path snake;
        bool first = true;
        for (float y = 350.0f; y <= 770.0f; y += 4.0f)
        {
            const float env = std::sin ((y - 350.0f) / 420.0f * juce::MathConstants<float>::pi);
            const float x = 830.0f + 52.0f * env * std::sin ((y - 350.0f) * 0.05f
                                                             + (float) phase * juce::MathConstants<float>::pi);
            if (first) { snake.startNewSubPath (x * s, y * s); first = false; }
            else        snake.lineTo (x * s, y * s);
        }
        g.setColour (phase == 0 ? Pal::bone.withAlpha (0.9f) : Pal::bone.withAlpha (0.5f));
        g.strokePath (snake, juce::PathStrokeType (2.2f * s));
    }

    g.setColour (Pal::blood.withAlpha (0.55f));
    g.drawEllipse (R (775, 300, 110, 110), 1.2f * s);
    g.drawEllipse (R (765, 580, 130, 130), 1.2f * s);

    g.setColour (Pal::blood);
    strokePentagram (g, { 830 * s, 800 * s }, 30.0f * s, 2.0f * s);

    // ---- Luna ----
    {
        const juce::Point<float> mc (1530 * s, 215 * s);
        const float mr = 55.0f * s;
        juce::ColourGradient moon (Pal::metalHi, mc.x - mr * 0.4f, mc.y - mr * 0.4f,
                                   Pal::metalLo, mc.x + mr, mc.y + mr, true);
        g.setGradientFill (moon);
        g.fillEllipse (mc.x - mr, mc.y - mr, 2 * mr, 2 * mr);
        g.setColour (Pal::bone.withAlpha (0.5f));
        g.drawEllipse (mc.x - mr, mc.y - mr, 2 * mr, 2 * mr, 1.5f * s);
        juce::Random r (7);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        for (int i = 0; i < 9; ++i)
        {
            const float cr = (4.0f + r.nextFloat() * 9.0f) * s;
            g.fillEllipse (mc.x + (r.nextFloat() - 0.5f) * mr * 1.2f,
                           mc.y + (r.nextFloat() - 0.5f) * mr * 1.2f, cr, cr);
        }
    }

    // ---- Ojo + pentagrama derecha, texto lateral ----
    strokeEyeTriangle (g, { 1478 * s, 380 * s }, 110 * s, 95 * s, 1.8f * s, Pal::bone, Pal::blood);
    g.setColour (Pal::blood);
    strokePentagram (g, { 1478 * s, 490 * s }, 30.0f * s, 2.0f * s);

    g.setFont (monoFont (21.0f * s));
    g.setColour (Pal::bone);
    g.drawText ("DARK",   R (1455, 527, 140, 26), juce::Justification::centredLeft);
    g.drawText ("SOUND",  R (1455, 553, 140, 26), juce::Justification::centredLeft);
    g.drawText ("RITUALS",R (1455, 579, 140, 26), juce::Justification::centredLeft);
    for (int i = 0; i < 40; ++i)   // codigo de barras
    {
        g.setColour (Pal::bone.withAlpha (0.8f));
        g.fillRect (R (1455.0f + (float) i * 3.6f, 614, (i % 3 == 0 ? 2.5f : 1.2f), 26));
    }

    // ---- Esquinas: pentagramas ----
    g.setColour (Pal::bone.withAlpha (0.8f));
    strokePentagram (g, { 62 * s, 872 * s }, 22.0f * s, 1.5f * s);
    strokePentagram (g, { 1595 * s, 872 * s }, 22.0f * s, 1.5f * s);
    g.setColour (Pal::blood);
    strokePentagram (g, { 145 * s, 505 * s }, 32.0f * s, 2.0f * s);

    // ---- Franja inferior ----
    g.setFont (monoFont (24.0f * s).withExtraKerningFactor (0.25f));
    g.setColour (Pal::blood);
    g.drawText ("C" + oSlash() + "SMIC N" + xi() + "URAL C" + oSlash() + "LLAPS" + xi(),
                R (228, 852, 520, 36), juce::Justification::centredLeft);
    g.drawText ("FREEY-TEK", R (1275, 852, 220, 36), juce::Justification::centredLeft);
    for (int i = 0; i < 38; ++i)
        g.fillRect (R (1140.0f + (float) i * 2.6f, 856, (i % 4 == 0 ? 2.0f : 1.0f), 30));
}

//==============================================================================
void FreeyTekAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / refW;

    g.fillAll (Pal::bg);

    if (artImage.isValid())
        g.drawImage (artImage, getLocalBounds().toFloat());
    else
        drawProceduralArt (g, s);

    // Nombre del algoritmo de distorsion seleccionado (bajo el caption de Type)
    static const char* names[] = { "FOLDBACK", "VALVE / TUBE", "DIODE / SATURATOR" };
    const int idx = juce::jlimit (0, 2, (int) std::round (typeSlider.getValue()));
    g.setFont (monoFont (22.0f * s, true));
    g.setColour (Pal::blood);
    g.drawText (juce::String ("> ") + names[idx],
                juce::Rectangle<float> (1095 * s, 580 * s, 330 * s, 30 * s),
                juce::Justification::centredLeft);

    // Textura de grano/rayones encima del arte
    if (texture.isValid())
        g.drawImageAt (texture, 0, 0);

    // Vineta
    juce::ColourGradient vig (juce::Colours::transparentBlack, getWidth() * 0.5f, getHeight() * 0.5f,
                              juce::Colours::black.withAlpha (0.65f), 0.0f, 0.0f, true);
    g.setGradientFill (vig);
    g.fillRect (getLocalBounds());
}
