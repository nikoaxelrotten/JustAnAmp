#pragma once
#include "PluginProcessor.h"

namespace jahead
{
    constexpr int designW = 1200;
    constexpr int designH = 810;

    // Un "skin" por head: color de acento, carácter del panel de controles y de las perillas.
    struct Skin
    {
        const char* name;
        juce::Colour accent;
        juce::Colour stripTop, stripBottom;
        juce::Colour knobHi, knobLo, pointer;
        juce::Colour label, value;
        bool light;          // panel de controles claro (texto oscuro)
    };

    const Skin& skinFor (int head);

    // Colores propios de cada modulo
    const juce::Colour colDyn    { 0xffff8a5c };
    const juce::Colour colPedal  { 0xfff0a030 };
    const juce::Colour colMod    { 0xffb36bf0 };
    const juce::Colour colDelay  { 0xff38d39f };
    const juce::Colour colReverb { 0xff7f8cff };
}

//==============================================================================
class JAHeadLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JAHeadLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getPopupMenuFont() override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool, bool) override;

    int skin = 0;   // head activo (0..3)
};

//==============================================================================
class JustAnAmpEditor : public juce::AudioProcessorEditor
{
public:
    explicit JustAnAmpEditor (JustAnAmpProcessor&);
    ~JustAnAmpEditor() override;
    void resized() override;

private:
    class Content;
    JustAnAmpProcessor& proc;
    JAHeadLookAndFeel laf;
    std::unique_ptr<Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustAnAmpEditor)
};
