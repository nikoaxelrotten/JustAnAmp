#include "PluginEditor.h"
#include "BinaryData.h"
#include <map>

using namespace jahead;
using APVTS = juce::AudioProcessorValueTreeState;

static juce::Font fnt (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
const Skin& jahead::skinFor (int head)
{
    static const Skin skins[6] =
    {
        // Pulga: frente amarillo, perillas negras con puntero blanco
        { "Pulga",          juce::Colour (0xfff2c230), juce::Colour (0xfff2c43a), juce::Colour (0xffd9a41a),
                            juce::Colour (0xff4d4a44), juce::Colour (0xff070707), juce::Colour (0xffffffff),
                            juce::Colour (0xff201803), juce::Colour (0xff6e4300), true },
        // NotFieldy: panel negro con neon verde y perillas facetadas
        { "NotFieldy",      juce::Colour (0xff52ee3a), juce::Colour (0xff131513), juce::Colour (0xff070807),
                            juce::Colour (0xffc4ffa4), juce::Colour (0xff1c8a14), juce::Colour (0xff06240a),
                            juce::Colour (0xffb9f2b0), juce::Colour (0xff7dff62), false },
        // RoAguMiSSTruVe: negro con perillas de tapa cromada
        { "RoAguMiSSTruVe", juce::Colour (0xffa9bccd), juce::Colour (0xff2a2c30), juce::Colour (0xff131416),
                            juce::Colour (0xfff4f6f8), juce::Colour (0xff737a83), juce::Colour (0xff101010),
                            juce::Colour (0xffc9ced6), juce::Colour (0xffffffff), false },
        // Ryandemar: panel plateado con perillas rojas facetadas
        { "Ryandemar",      juce::Colour (0xffd1283c), juce::Colour (0xffdcdee2), juce::Colour (0xffb4b8be),
                            juce::Colour (0xffe8586a), juce::Colour (0xff5c0914), juce::Colour (0xffffffff),
                            juce::Colour (0xff1f2024), juce::Colour (0xffa01828), true },
        // Canciller: dos unidades de rack (limpio / sucio), acento turquesa
        { "Canciller",      juce::Colour (0xff19c3d6), juce::Colour (0xff20262b), juce::Colour (0xff0d1013),
                            juce::Colour (0xff8aeaf5), juce::Colour (0xff0b5b66), juce::Colour (0xff04222a),
                            juce::Colour (0xffb4e4ea), juce::Colour (0xff5fe0f0), false },
        // TimmyC: tolex quemado y tela naranja, perillas de tapa cromada
        { "TimmyC",         juce::Colour (0xffff5a1f), juce::Colour (0xff25201e), juce::Colour (0xff100d0c),
                            juce::Colour (0xfff4f6f8), juce::Colour (0xff737a83), juce::Colour (0xff101010),
                            juce::Colour (0xffecd5c7), juce::Colour (0xffff8a55), false }
    };
    return skins[juce::jlimit (0, 5, head)];
}

//==============================================================================
struct KnobSlider : public juce::Slider
{
    bool ampStyle = false;
    juce::Colour col { 0xffffffff };
};

JAHeadLookAndFeel::JAHeadLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xffa7adb8));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff101114));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff5a626d));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15171c));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd5d9df));
    setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xff8a919c));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2c3038));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

juce::Font JAHeadLookAndFeel::getTextButtonFont (juce::TextButton& b, int) { return fnt (b.getProperties().contains ("small") ? 11.0f : 12.0f, juce::Font::bold); }

juce::Font JAHeadLookAndFeel::getPopupMenuFont() { return fnt (13.0f); }

void JAHeadLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? b.findColour (juce::TextButton::buttonOnColourId)
                    : juce::Colour (0xff1d2026).brighter (down ? 0.45f : (over ? 0.12f : 0.0f)));
    g.fillRoundedRectangle (r, 6.0f);
    if (! on)
    {
        g.setColour (juce::Colour (0xff2e323a));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        if (b.getProperties().contains ("hint"))
        {
            g.setColour (juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["hint"]));
            g.fillRoundedRectangle (r.getX() + 8.0f, r.getBottom() - 4.0f, r.getWidth() - 16.0f, 2.5f, 1.2f);
        }
    }
}

void JAHeadLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                          juce::Slider::SliderStyle, juce::Slider&)
{
    const auto& sk = skinFor (skin);
    const float cx = (float) x + (float) w * 0.5f;
    g.setColour (juce::Colour (0xff050605));
    g.fillRoundedRectangle (cx - 3.0f, (float) y, 6.0f, (float) h, 3.0f);
    g.setColour (sk.accent.withAlpha (0.55f));
    for (int i = 0; i <= 8; ++i)
        g.drawHorizontalLine ((int) ((float) y + (float) h * (float) i / 8.0f), cx - 9.0f, cx - 5.0f);
    g.setColour (sk.accent.withAlpha (0.20f));
    g.fillRoundedRectangle (cx - 14.0f, pos - 8.0f, 28.0f, 16.0f, 4.0f);
    g.setGradientFill (juce::ColourGradient (sk.knobHi, cx, pos - 6.0f, sk.accent.darker (0.5f), cx, pos + 6.0f, false));
    g.fillRoundedRectangle (cx - 11.0f, pos - 6.0f, 22.0f, 12.0f, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (cx - 9.0f, pos, cx + 9.0f, pos, 1.4f);
}

void JAHeadLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool, bool)
{
    g.setColour (juce::Colour (0xff6a717a));
    for (int i = 0; i < 3; ++i)
    {
        const float o = 5.0f + (float) i * 5.0f;
        g.drawLine ((float) w - 3.0f, (float) h - o, (float) w - o, (float) h - 3.0f, 1.6f);
    }
}

// Perillas inspiradas en las fotos de cada head
static void drawAmpKnob (juce::Graphics& g, int skin, const Skin& sk, juce::Point<float> c, float d, float angle)
{
    const float R = d * 0.5f;
    const float sx = std::sin (angle), sy = -std::cos (angle);
    auto circle = [&] (float r) { return juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };
    auto lineAt = [&] (float r0, float r1, float th) { return juce::Line<float> (c.x + std::sin (th) * r0, c.y - std::cos (th) * r0, c.x + std::sin (th) * r1, c.y - std::cos (th) * r1); };
    auto facet = [&] (float r, int n)
    {
        juce::Path p;
        for (int i = 0; i < n; ++i)
        {
            const float th = (float) i * juce::MathConstants<float>::twoPi / (float) n;
            const juce::Point<float> q (c.x + std::sin (th) * r, c.y - std::cos (th) * r);
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        p.closeSubPath();
        return p;
    };

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (circle (R * 1.04f).translated (0.0f, 2.5f));

    const int style = skin == 0 ? 0 : ((skin == 2 || skin == 5) ? 2 : 1);
    if (style == 0)   // Pulga: negra con falda estriada y cuerpo brillante
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x - R * 0.5f, c.y - R * 0.6f, juce::Colour (0xff050505), c.x + R * 0.6f, c.y + R * 0.7f, false));
        g.fillEllipse (circle (R * 1.04f));
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        for (int i = 0; i < 30; ++i)
            g.drawLine (lineAt (R * 0.86f, R * 1.02f, (float) i * juce::MathConstants<float>::twoPi / 30.0f), 1.0f);
        g.setGradientFill (juce::ColourGradient (sk.knobHi, c.x - R * 0.3f, c.y - R * 0.4f, sk.knobLo, c.x + R * 0.5f, c.y + R * 0.6f, true));
        g.fillEllipse (circle (R * 0.66f));
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawEllipse (circle (R * 0.62f), 1.0f);
        g.setColour (sk.pointer);
        g.drawLine (lineAt (R * 0.12f, R * 1.0f, angle), 3.4f);
    }
    else if (style == 2)   // RoAguMiSSTruVe / TimmyC: base negra y tapa cromada
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2e), c.x - R * 0.5f, c.y - R * 0.6f, juce::Colour (0xff060606), c.x + R * 0.6f, c.y + R * 0.7f, false));
        g.fillEllipse (circle (R * 1.04f));
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        for (int i = 0; i < 40; ++i)
            g.drawLine (lineAt (R * 0.9f, R * 1.03f, (float) i * juce::MathConstants<float>::twoPi / 40.0f), 1.0f);
        g.setGradientFill (juce::ColourGradient (sk.knobHi, c.x - R * 0.4f, c.y - R * 0.5f, sk.knobLo, c.x + R * 0.6f, c.y + R * 0.7f, true));
        g.fillEllipse (circle (R * 0.72f));
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawEllipse (circle (R * 0.72f), 1.0f);
        g.setColour (juce::Colour (0xffffffff));
        g.drawLine (lineAt (R * 0.78f, R * 1.0f, angle), 3.0f);
        g.setColour (sk.pointer);
        g.drawLine (lineAt (R * 0.1f, R * 0.66f, angle), 2.4f);
    }
    else   // NotFieldy (verde) y Ryandemar (rojo): perilla facetada
    {
        if (skin == 1 || skin == 4)
        {
            g.setColour (sk.accent.withAlpha (0.22f));
            g.fillEllipse (circle (R * 1.16f));
        }
        const auto body = facet (R * 1.0f, 10);
        g.setGradientFill (juce::ColourGradient (sk.knobHi, c.x - R * 0.5f, c.y - R * 0.6f, sk.knobLo, c.x + R * 0.7f, c.y + R * 0.8f, false));
        g.fillPath (body);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.strokePath (body, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.strokePath (facet (R * 0.78f, 10), juce::PathStrokeType (1.0f));
        g.setColour (sk.pointer);
        g.drawLine (lineAt (R * 0.18f, R * 0.95f, angle), 3.6f);
    }
    juce::ignoreUnused (sx, sy);
}

void JAHeadLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float a0, float a1, juce::Slider& s)
{
    auto* ks = dynamic_cast<KnobSlider*> (&s);
    const bool amp = ks != nullptr && ks->ampStyle;
    const auto& sk = skinFor (skin);
    const juce::Colour ringCol = amp ? sk.accent : (ks != nullptr ? ks->col : juce::Colours::white);

    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.0f);
    const float D = juce::jmin (b.getWidth(), b.getHeight());
    auto outer = juce::Rectangle<float> (D, D).withCentre (b.getCentre());
    const auto c = outer.getCentre();
    const float angle = a0 + pos * (a1 - a0);

    // Anillo de valor
    const float ringR = D * 0.5f - 2.5f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a1, true);
    value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, angle, true);
    g.setColour (amp && sk.light ? juce::Colour (0x38000000) : juce::Colour (0x20ffffff));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (amp)
    {
        g.setColour (ringCol.withAlpha (0.25f));
        g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (ringCol);
    g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (amp)
    {
        drawAmpKnob (g, skin, sk, c, D * 0.72f, angle);
        return;
    }

    // Cuerpo
    auto body = outer.reduced (D * 0.14f);
    const float d = body.getWidth();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (body.translated (0.0f, 2.5f));
    if (amp)
        g.setGradientFill (juce::ColourGradient (sk.knobHi, body.getX() + d * 0.3f, body.getY() + d * 0.25f,
                                                 sk.knobLo, body.getRight(), body.getBottom(), true));
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff484c54), body.getX() + d * 0.3f, body.getY() + d * 0.25f,
                                                 juce::Colour (0xff121417), body.getRight(), body.getBottom(), true));
    g.fillEllipse (body);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (body, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawEllipse (body.reduced (1.5f), 1.0f);

    // Indicador
    const float r0 = d * 0.18f, r1 = d * 0.46f;
    const float sx = std::sin (angle), sy = -std::cos (angle);
    g.setColour (amp ? sk.pointer : juce::Colours::white);
    g.drawLine (c.x + sx * r0, c.y + sy * r0, c.x + sx * r1, c.y + sy * r1, amp ? 3.2f : 2.6f);
}

//==============================================================================
// Una perilla con nombre y valor
class KnobUnit : public juce::Component
{
public:
    KnobUnit (APVTS& apvts, const juce::String& pid, const juce::String& nm, int size, bool amp,
              juce::Colour modCol, std::function<juce::String (double)> f)
        : name (nm), knobSize (size), fmt (std::move (f)), labelCol (0xff9aa0aa), valueCol (modCol)
    {
        slider.ampStyle = amp;
        slider.col = modCol;
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (190);
        slider.setScrollWheelEnabled (true);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (apvts, pid, slider);
        if (auto* prm = apvts.getParameter (pid))
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        slider.onValueChange = [this] { repaint(); };
        setTitle (nm);
    }

    void setColours (juce::Colour label, juce::Colour value) { labelCol = label; valueCol = value; repaint(); }
    void setActive (bool a) { slider.setEnabled (a); setAlpha (a ? 1.0f : 0.32f); }
    void setName (const juce::String& n) { name = n; repaint(); }

    void resized() override { slider.setBounds ((getWidth() - knobSize) / 2, 0, knobSize, knobSize); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (labelCol);
        g.setFont (fnt (11.5f, juce::Font::bold));
        g.drawText (name, 0, knobSize + 2, getWidth(), 14, juce::Justification::centred);
        g.setColour (valueCol);
        g.setFont (fnt (11.5f));
        g.drawText (fmt (slider.getValue()), 0, knobSize + 16, getWidth(), 14, juce::Justification::centred);
    }

    int totalHeight() const { return knobSize + 31; }

private:
    KnobSlider slider;
    juce::String name;
    int knobSize;
    std::function<juce::String (double)> fmt;
    juce::Colour labelCol, valueCol;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

static juce::String fmt1 (double v)  { return juce::String (v, 1); }
static juce::String fmtDb (double v) { return juce::String (v, 1) + " dB"; }
static juce::String fmtPct (double v){ return juce::String ((int) std::round (v)) + "%"; }
static juce::String fmtGate (double v) { return v <= -79.5 ? juce::String ("OFF") : juce::String (v, 1) + " dB"; }
static juce::String fmtRatio (double v) { return juce::String (v, 1) + ":1"; }
static juce::String fmtMs1 (double v) { return v < 10.0 ? juce::String (v, 1) + " ms" : juce::String ((int) std::round (v)) + " ms"; }
static juce::String fmtFreq (double v)
{
    static const char* f[5] = { "220 Hz", "450 Hz", "800 Hz", "1.6 kHz", "3 kHz" };
    return f[juce::jlimit (0, 4, (int) std::round (v))];
}
static juce::String fmtMs (double v) { return juce::String ((int) std::round (v)) + " ms"; }

//==============================================================================
// Fila de botones ligada a un parámetro de tipo lista
class ChoiceStrip : public juce::Component
{
public:
    ChoiceStrip (APVTS& apvts, const juce::String& pid, const juce::StringArray& names,
                 std::vector<juce::Colour> accents, int numCols, int rowHeight, int gapPx, int radioId, bool hint)
        : cols (numCols), rowH (rowHeight), gap (gapPx)
    {
        auto* param = apvts.getParameter (pid);
        for (int i = 0; i < names.size(); ++i)
        {
            auto b = std::make_unique<juce::TextButton> (names[i]);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (radioId);
            const auto ac = accents[(size_t) juce::jmin (i, (int) accents.size() - 1)];
            b->setColour (juce::TextButton::buttonOnColourId, ac);
            b->setColour (juce::TextButton::textColourOnId, ac.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff101114) : juce::Colours::white);
            if (hint) b->getProperties().set ("hint", (juce::int64) ac.getARGB());
            b->onClick = [this, i] { att->setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (*b);
            buttons.push_back (std::move (b));
        }
        att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            const int idx = (int) std::round (v);
            for (size_t k = 0; k < buttons.size(); ++k)
                buttons[k]->setToggleState ((int) k == idx, juce::dontSendNotification);
            if (onChange) onChange (idx);
        });
    }

    void start() { att->sendInitialUpdate(); }

    void resized() override
    {
        const int n = (int) buttons.size();
        const int bw = (getWidth() - gap * (cols - 1)) / cols;
        for (int i = 0; i < n; ++i)
            buttons[(size_t) i]->setBounds ((i % cols) * (bw + gap), (i / cols) * (rowH + gap), bw, rowH);
    }

    std::function<void (int)> onChange;

private:
    int cols, rowH, gap;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    std::unique_ptr<juce::ParameterAttachment> att;
};

//==============================================================================
// Boton que escribe 0 / 10 en un parametro float (para los "switches" Ultra Lo / Ultra Hi)
class FloatToggle : public juce::TextButton
{
public:
    FloatToggle (APVTS& apvts, const juce::String& pid, const juce::String& text)
    {
        setButtonText (text);
        setClickingTogglesState (true);
        att = std::make_unique<juce::ParameterAttachment> (*apvts.getParameter (pid), [this] (float v)
        {
            setToggleState (v > 5.0f, juce::dontSendNotification);
        });
        onClick = [this] { att->setValueAsCompleteGesture (getToggleState() ? 10.0f : 0.0f); };
        att->sendInitialUpdate();
    }
private:
    std::unique_ptr<juce::ParameterAttachment> att;
};

//==============================================================================
// Caja de un modulo (panel con titulo). Los modulos de efectos se arrastran por el encabezado.
class ModuleBox : public juce::Component
{
public:
    ModuleBox (const juce::String& t, juce::Colour c, bool canDrag) : title (t), col (c), draggable (canDrag) {}

    std::function<void (juce::Graphics&, juce::Rectangle<int>)> extraPaint;
    std::function<void (ModuleBox&, const juce::MouseEvent&)> onDragStart, onDragMove, onDragEnd;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff15171c));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff262930));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
        juce::Path top;
        top.addRoundedRectangle (0.0f, 0.0f, r.getWidth(), 4.0f, 10.0f, 10.0f, true, true, false, false);
        g.setColour (col);
        g.fillPath (top);
        g.setFont (fnt (12.5f, juce::Font::bold));
        g.drawText (title, draggable ? 22 : 18, 12, 200, 18, juce::Justification::centredLeft);
        if (draggable)
        {
            g.setColour (juce::Colour (0xff5d636d));
            for (int cx = 0; cx < 2; ++cx)
                for (int cy = 0; cy < 3; ++cy)
                    g.fillEllipse (8.0f + (float) cx * 5.0f, 14.0f + (float) cy * 5.0f, 2.6f, 2.6f);
        }
        if (extraPaint) extraPaint (g, getLocalBounds());
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (draggable && e.y < 34 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (draggable && e.y < 34) { dragging = true; if (onDragStart) onDragStart (*this, e); }
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragging && onDragMove) onDragMove (*this, e); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragging) { dragging = false; if (onDragEnd) onDragEnd (*this, e); }
    }

private:
    juce::String title;
    juce::Colour col;
    bool draggable, dragging = false;
};

//==============================================================================
class JustAnAmpEditor::Content : public juce::Component, private juce::Timer
{
public:
    Content (JustAnAmpProcessor& p, JAHeadLookAndFeel& l, std::function<void (float)> scaleCb)
        : proc (p), laf (l), setScale (std::move (scaleCb))
    {
        const void* data[6]  = { BinaryData::amp_pulga_jpg, BinaryData::amp_notfieldy_jpg, BinaryData::amp_roagu_jpg, BinaryData::amp_ryan_jpg,
                                 BinaryData::amp_canciller_jpg, BinaryData::amp_timmyc_jpg };
        const int   sizes[6] = { BinaryData::amp_pulga_jpgSize, BinaryData::amp_notfieldy_jpgSize, BinaryData::amp_roagu_jpgSize, BinaryData::amp_ryan_jpgSize,
                                 BinaryData::amp_canciller_jpgSize, BinaryData::amp_timmyc_jpgSize };
        for (int i = 0; i < 6; ++i) heads[i] = juce::ImageCache::getFromMemory (data[i], sizes[i]);

        // --- Presets
        prevBtn.setButtonText ("<"); nextBtn.setButtonText (">"); presetBtn.setButtonText ("");
        presetBtn.getProperties().set ("small", true);
        prevBtn.setBounds (186, 15, 24, 30); presetBtn.setBounds (214, 15, 140, 30); nextBtn.setBounds (358, 15, 24, 30);
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (1); };
        presetBtn.onClick = [this] { showPresetMenu(); };
        for (auto* b : { &prevBtn, &presetBtn, &nextBtn }) addAndMakeVisible (*b);
        lastPreset = proc.getCurrentProgram();
        presetBtn.setButtonText (jah::presetName (lastPreset));
        startTimerHz (30);

        // --- Selector de head
        std::vector<juce::Colour> headCols;
        for (int i = 0; i < 6; ++i) headCols.push_back (skinFor (i).accent);
        headStrip = std::make_unique<ChoiceStrip> (p.apvts, "head", juce::StringArray { "Pulga", "NotFieldy", "RoAguMiSSTruVe", "Ryandemar", "Canciller", "TimmyC" },
                                                   headCols, 6, 34, 8, 201, true);
        headStrip->setBounds (24, 56, 1152, 34);
        addAndMakeVisible (*headStrip);

        // --- Gate + compresor
        dynBox = std::make_unique<ModuleBox> ("GATE + COMPRESSOR", colDyn, false);
        dynBox->setBounds (24, 108, 612, 190);
        dynBox->extraPaint = [this] (juce::Graphics& g, juce::Rectangle<int>)
        {
            g.setColour (juce::Colour (0xff2a2d34));
            g.drawVerticalLine (158, 44.0f, 176.0f);
            g.setColour (juce::Colour (0xff6c727c)); g.setFont (fnt (11.0f, juce::Font::bold));
            g.drawText ("GATE", 12, 40, 60, 16, juce::Justification::centredLeft);
            g.drawText ("GR", 424, 40, 22, 14, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff0d0e11));
            g.fillRoundedRectangle (448.0f, 42.0f, 120.0f, 10.0f, 3.0f);
            const float f = juce::jlimit (0.0f, 1.0f, grDb / 20.0f);
            g.setColour (juce::Colour (0xffff8a5c));
            g.fillRoundedRectangle (568.0f - 120.0f * f, 42.0f, 120.0f * f, 10.0f, 3.0f);
            g.setColour (juce::Colour (0xff868c96)); g.setFont (fnt (10.0f));
            g.drawText (juce::String (grDb, 1) + " dB", 448, 54, 120, 12, juce::Justification::centredRight);
        };
        addAndMakeVisible (*dynBox);
        addKnob (*dynBox, "gthr", "THRESH",  8, 78, 70, 54, false, colDyn, fmtGate);
        addKnob (*dynBox, "grel", "RELEASE", 80, 78, 70, 54, false, colDyn, fmtMs);
        compStrip = std::make_unique<ChoiceStrip> (p.apvts, "ctype", juce::StringArray { "Off", "Opto", "FET" },
                                                   std::vector<juce::Colour> { colDyn }, 3, 28, 6, 205, false);
        compStrip->setBounds (168, 36, 240, 28);
        dynBox->addAndMakeVisible (*compStrip);
        {
            const char* id[] = { "cthr", "cratio", "catk", "crel", "cblend", "cgain" };
            const char* nm[] = { "THRESH", "RATIO", "ATTACK", "RELEASE", "BLEND", "MAKEUP" };
            std::function<juce::String (double)> fm[] = { fmtDb, fmtRatio, fmtMs1, fmtMs, fmtPct, fmtDb };
            for (int i = 0; i < 6; ++i)
                compKnobs.push_back (addKnob (*dynBox, id[i], nm[i], 164 + i * 74, 78, 72, 54, false, colDyn, fm[i]));
        }

        // --- Pedal
        pedBox = std::make_unique<ModuleBox> ("PEDAL", colPedal, false);
        pedBox->setBounds (648, 108, 528, 190);
        pedBox->extraPaint = [this] (juce::Graphics& g, juce::Rectangle<int>)
        {
            g.setColour (juce::Colour (0xff6c727c)); g.setFont (fnt (11.0f));
            const char* d[] = { "Sin pedal", "Preamp valvular con mezcla Blend", "Distorsi\xc3\xb3n en paralelo: graves intactos" };
            g.drawText (juce::String::fromUTF8 (d[pedalIdx]), 330, 40, 190, 28, juce::Justification::centredLeft);
            if (skin >= 4) g.drawText (juce::String::fromUTF8 ("Pedal: solo en el amp sucio"), 330, 62, 190, 12, juce::Justification::centredLeft);
        };
        addAndMakeVisible (*pedBox);
        pedStrip = std::make_unique<ChoiceStrip> (p.apvts, "pedal", juce::StringArray { "Off", "SansAmp", "Darkglass" },
                                                  std::vector<juce::Colour> { colPedal }, 3, 28, 6, 202, false);
        pedStrip->setBounds (18, 36, 300, 28);
        pedBox->addAndMakeVisible (*pedStrip);
        {
            const char* id[] = { "pdrive", "pblend", "plevel", "pbass", "ptreble", "pextra" };
            const char* nm[] = { "DRIVE", "BLEND", "LEVEL", "BASS", "TREBLE", "PRESENCE" };
            for (int i = 0; i < 6; ++i)
                pedKnobs.push_back (addKnob (*pedBox, id[i], nm[i], 12 + i * 84, 78, 82, 58, false, colPedal, i == 2 ? fmtDb : fmt1));
        }

        // --- Controles del amp (se acomodan segun el head)
        {
            const char* id[] = { "gain", "bass", "mid", "himid", "treble", "contour", "ulo", "uhi", "freq", "dirty", "master" };
            for (auto* pid : id)
                ampKnobs[pid] = addKnob (*this, pid, pid, 0, 0, 100, 64, true, juce::Colours::white,
                                         juce::String (pid) == "master" ? fmtDb : (juce::String (pid) == "freq" ? fmtFreq : fmt1));
        }
        boostBtn.setButtonText ("BOOST"); brightBtn.setButtonText ("BRIGHT");
        for (auto* b : { &boostBtn, &brightBtn }) { b->setClickingTogglesState (true); addChildComponent (*b); }
        boostAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "boost", boostBtn);
        brightAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "bright", brightBtn);
        uloTog = std::make_unique<FloatToggle> (p.apvts, "ulo", "ULTRA LO");
        uhiTog = std::make_unique<FloatToggle> (p.apvts, "uhi", "ULTRA HI");
        addChildComponent (*uloTog); addChildComponent (*uhiTog);
        for (int i = 0; i < 7; ++i)
        {
            auto& s = eqSl[(size_t) i];
            s.setSliderStyle (juce::Slider::LinearVertical);
            s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
            s.setScrollWheelEnabled (true);
            s.setDoubleClickReturnValue (true, 0.0);
            addChildComponent (s);
            eqAtt.push_back (std::make_unique<APVTS::SliderAttachment> (p.apvts, "eq" + juce::String (i + 1), s));
        }

        // --- Efectos: 7 modulos, cada uno con su on/off; se arrastran por el encabezado para cambiar el orden
        struct FxKnob { const char* pid; const char* label; std::function<juce::String (double)> fmt; };
        auto makeFx = [this] (int id, const char* title, juce::Colour col, const char* onId, std::vector<FxKnob> ks)
        {
            auto b = std::make_unique<ModuleBox> (title, col, true);
            b->setSize (fxW, fxH);
            int slot = 0;
            for (const auto& k : ks)
            {
                fxKnobs[id].push_back (addKnob (*b, k.pid, k.label, 4 + (slot % 2) * 80, 42 + (slot / 2) * 78, 72, 46, false, col, k.fmt));
                ++slot;
            }
            setupPower (*b, fxOn[id], onId, col, fxKnobs[id], nullptr);
            byId[id] = b.get();
            fxBox[id] = std::move (b);
        };
        makeFx (0, "CHORUS",     colChorus,  "chon", { { "chrate", "RATE", fmt1 }, { "chdepth", "DEPTH", fmt1 }, { "chmix", "MIX", fmtPct } });
        makeFx (1, "FLANGER",    colFlanger, "flon", { { "flrate", "RATE", fmt1 }, { "fldepth", "DEPTH", fmt1 }, { "flfb", "FEEDBACK", fmt1 }, { "flmix", "MIX", fmtPct } });
        makeFx (2, "PHASER",     colPhaser,  "phon", { { "phrate", "RATE", fmt1 }, { "phdepth", "DEPTH", fmt1 }, { "phfb", "FEEDBACK", fmt1 }, { "phmix", "MIX", fmtPct } });
        makeFx (3, "OCTAVER",    colOct,     "ocon", { { "ocdown", "OCT -1", fmt1 }, { "ocup", "OCT +1", fmt1 }, { "octone", "TONE", fmt1 }, { "ocdirect", "DIRECT", fmtPct } });
        makeFx (4, "ENV FILTER", colEnv,     "efon", { { "efsens", "SENS", fmt1 }, { "efrange", "RANGE", fmt1 }, { "efres", "RES", fmt1 }, { "efmix", "MIX", fmtPct } });
        makeFx (5, "DELAY",      colDelay,   "don",  { { "dtime", "TIME", fmtMs }, { "dfb", "FEEDBACK", fmtPct }, { "dmix", "MIX", fmtPct } });
        makeFx (6, "REVERB",     colReverb,  "ron",  { { "rsize", "SIZE", fmt1 }, { "rdamp", "DAMP", fmt1 }, { "rmix", "MIX", fmtPct } });

        tapButton.setButtonText ("TAP");
        tapButton.getProperties().set ("hint", (juce::int64) colDelay.getARGB());
        tapButton.setBounds (88, 134, 66, 32);
        tapButton.onClick = [this] { tap(); };
        byId[5]->addAndMakeVisible (tapButton);
        {
            auto prev = fxOn[5].onStateChange;
            fxOn[5].onStateChange = [this, prev] { if (prev) prev(); tapButton.setEnabled (fxOn[5].getToggleState()); };
            tapButton.setEnabled (fxOn[5].getToggleState());
        }

        for (int id = 0; id < fxN; ++id)
        {
            addAndMakeVisible (*byId[id]);
            byId[id]->onDragStart = [this, id] (ModuleBox& b, const juce::MouseEvent& e) { dragStart (id, b, e); };
            byId[id]->onDragMove  = [this, id] (ModuleBox& b, const juce::MouseEvent& e) { dragMove (id, b, e); };
            byId[id]->onDragEnd   = [this, id] (ModuleBox& b, const juce::MouseEvent& e) { dragEnd (id, b, e); };
        }
        orderAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("fxorder7"), [this] (float v)
        {
            jah::decodeOrder ((int) std::round (v), order);
            if (dragId < 0) layoutModules (initialised, -1);
        });
        orderAtt->sendInitialUpdate();
        initialised = true;

        // --- Escala
        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        const char* labels[] = { "75%", "100%", "125%", "150%" };
        for (int i = 0; i < 4; ++i)
        {
            auto& b = scaleButtons[(size_t) i];
            b.setButtonText (labels[i]);
            b.setClickingTogglesState (true);
            b.setRadioGroupId (101);
            b.setBounds (940 + i * 58, 16, 52, 30);
            const float s = scales[i];
            b.onClick = [this, s] { setScale (s); };
            addAndMakeVisible (b);
        }

        headStrip->onChange = [this] (int i) { applySkin (i); };
        pedStrip->onChange = [this] (int i)
        {
            pedalIdx = i;
            pedKnobs[5]->setName (i == 2 ? "GRUNT" : "PRESENCE");
            for (auto* k : pedKnobs) k->setActive (i != 0);
            pedBox->repaint();
        };
        compStrip->onChange = [this] (int i)
        {
            for (size_t k = 0; k < compKnobs.size(); ++k)
                compKnobs[k]->setActive (i != 0 && ! (k == 1 && i == 1));   // el ratio del Opto es fijo
        };
        headStrip->start();
        pedStrip->start();
        compStrip->start();
    }

    ~Content() override { stopTimer(); }

    void updateScaleHighlight (float s)
    {
        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        for (int i = 0; i < 4; ++i)
            scaleButtons[(size_t) i].setToggleState (std::abs (scales[i] - s) < 0.02f, juce::dontSendNotification);
    }

    void applySkin (int s)
    {
        skin = juce::jlimit (0, 5, s);
        laf.skin = skin;
        const auto& sk = skinFor (skin);
        for (auto& kv : ampKnobs) kv.second->setColours (sk.label, sk.value);
        for (auto* b : { (juce::TextButton*) &boostBtn, (juce::TextButton*) &brightBtn, (juce::TextButton*) uloTog.get(), (juce::TextButton*) uhiTog.get() })
        {
            b->setColour (juce::TextButton::buttonOnColourId, sk.accent);
            b->setColour (juce::TextButton::textColourOnId, sk.accent.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff101114) : juce::Colours::white);
        }
        layoutAmp();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& sk = skinFor (skin);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1c21), 600.0f, 0.0f, juce::Colour (0xff0b0c0f), 600.0f, (float) designH, false));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (sk.accent.withAlpha (0.13f), 600.0f, -40.0f, sk.accent.withAlpha (0.0f), 600.0f, 420.0f, true));
        g.fillAll();
        g.setColour (juce::Colour (0xff2a2d34));
        g.drawRect (getLocalBounds(), 1);

        // Logo
        {
            const auto f = fnt (22.0f, juce::Font::bold);
            auto width = [&f] (const juce::String& t)
            {
                juce::GlyphArrangement ga;
                ga.addLineOfText (f, t, 0.0f, 0.0f);
                return ga.getBoundingBox (0, -1, true).getWidth() + 3.0f;
            };
            float x = 26.0f;
            g.setFont (f);
            const juce::String parts[] = { "JUST ", "AN", " AMP" };
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (i == 1 ? sk.accent : juce::Colours::white);
                g.drawText (parts[i], juce::Rectangle<float> (x, 8.0f, 200.0f, 30.0f), juce::Justification::centredLeft);
                x += width (parts[i]);
            }
            g.setColour (juce::Colour (0xff868c96));
            g.setFont (fnt (10.5f, juce::Font::bold));
            g.drawText ("BASS AMP SIM  -  VST3", 28, 36, 150, 14, juce::Justification::centredLeft);
            drawMeter (g, "IN", 442, 11, 300, 11, inDb, holdIn);
            drawMeter (g, "OUT", 442, 30, 300, 11, outDb, holdOut);
            g.drawText ("ESCALA", 862, 16, 70, 30, juce::Justification::centredRight);
        }

        // --- Amp: banner del head + panel de controles
        {
            const juce::Rectangle<float> amp (24.0f, (float) ampY, 1152.0f, 296.0f);
            const float imgH = 140.0f;
            const juce::Rectangle<float> imgR (amp.getX(), amp.getY(), amp.getWidth(), imgH);
            juce::Path clip;
            clip.addRoundedRectangle (amp.getX(), amp.getY(), amp.getWidth(), amp.getHeight(), 10.0f, 10.0f, true, true, true, true);

            g.saveState();
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (heads[skin], imgR, juce::RectanglePlacement::stretchToFit);

            const juce::Rectangle<float> strip (amp.getX(), imgR.getBottom(), amp.getWidth(), amp.getBottom() - imgR.getBottom());
            g.setGradientFill (juce::ColourGradient (sk.stripTop, 0.0f, strip.getY(), sk.stripBottom, 0.0f, strip.getBottom(), false));
            g.fillRect (strip);
            g.setColour ((sk.light ? juce::Colours::black : juce::Colours::white).withAlpha (0.04f));
            for (float yy = strip.getY() + 2.0f; yy < strip.getBottom(); yy += 3.0f)
                g.drawHorizontalLine ((int) yy, strip.getX(), strip.getRight());
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (strip.getX(), strip.getY(), strip.getWidth(), 2.0f);
            g.setColour (sk.accent.withAlpha (0.85f));
            g.fillRect (strip.getX(), strip.getY() + 2.0f, strip.getWidth(), 1.5f);
            g.restoreState();

            g.setColour (juce::Colour (0xff262930));
            g.drawRoundedRectangle (amp.reduced (0.5f), 10.0f, 1.0f);

            const juce::Rectangle<float> tag (amp.getX() + 74.0f, amp.getY() + 10.0f, 200.0f, 22.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (tag, 11.0f);
            g.setColour (sk.accent);
            g.setFont (fnt (11.0f, juce::Font::bold));
            g.drawText (juce::String ("AMP  -  ") + juce::String (sk.name).toUpperCase(), tag.toNearestInt(), juce::Justification::centred);

            if (skin == 1)   // etiquetas del ecualizador grafico
            {
                static const char* fq[7] = { "45", "110", "280", "700", "1.6k", "3.6k", "8k" };
                g.setColour (sk.label);
                g.setFont (fnt (10.5f, juce::Font::bold));
                for (int i = 0; i < 7; ++i)
                    g.drawText (fq[i], (int) (24.0f + 700.0f + 66.0f * (float) i - 30.0f), ampY + 140 + 128, 60, 14, juce::Justification::centred);
                g.drawText ("GRAPHIC EQ", (int) (24.0f + 700.0f - 40.0f), ampY + 140 + 6, 200, 14, juce::Justification::centredLeft);
            }
        }
    }

private:
    static constexpr int ampY = 310;
    static constexpr int fxN = 7, fxW = 160, fxH = 205, rowY = 618, rowX0 = 24, fxGap = 5;

    // Disposicion de controles del amp por head: kind 0 perilla, 1 boton, 2 fader del grafico
    struct Item { const char* pid; const char* label; int kind; float cx; float dy; };
    static std::vector<Item> ampLayout (int head)
    {
        switch (head)
        {
            case 0:   // Pulga
                return { { "gain", "GAIN", 0, 72, 0 }, { "bass", "BASS", 0, 216, 0 }, { "mid", "LO MID", 0, 360, 0 }, { "himid", "HI MID", 0, 504, 0 },
                         { "treble", "TREBLE", 0, 648, 0 }, { "contour", "CONTOUR", 0, 792, 0 }, { "master", "MASTER", 0, 936, 0 }, { "boost", "BOOST", 1, 1080, 20 } };
            case 1:   // NotFieldy
                return { { "gain", "GAIN", 0, 70, 0 }, { "bass", "BASS", 0, 180, 0 }, { "mid", "MID", 0, 290, 0 }, { "treble", "TREBLE", 0, 400, 0 },
                         { "master", "MASTER", 0, 510, 0 }, { "bright", "BRIGHT", 1, 612, 20 },
                         { "eq1", "", 2, 700, 0 }, { "eq2", "", 2, 766, 0 }, { "eq3", "", 2, 832, 0 }, { "eq4", "", 2, 898, 0 },
                         { "eq5", "", 2, 964, 0 }, { "eq6", "", 2, 1030, 0 }, { "eq7", "", 2, 1096, 0 } };
            case 2:   // RoAguMiSSTruVe
                return { { "gain", "GAIN", 0, 82, 0 }, { "ulo", "ULTRA LO", 1, 246, 0 }, { "uhi", "ULTRA HI", 1, 246, 46 },
                         { "bass", "BASS", 0, 410, 0 }, { "mid", "MID", 0, 574, 0 }, { "freq", "FREQ", 0, 738, 0 },
                         { "treble", "TREBLE", 0, 902, 0 }, { "master", "MASTER", 0, 1066, 0 } };
            case 4:   // Canciller: dos GK en paralelo
                return { { "gain", "GAIN", 0, 64, 0 }, { "bass", "BASS", 0, 192, 0 }, { "mid", "LO MID", 0, 320, 0 }, { "himid", "HI MID", 0, 448, 0 },
                         { "treble", "TREBLE", 0, 576, 0 }, { "contour", "CONTOUR", 0, 704, 0 }, { "dirty", "DIRTY MIX", 0, 832, 0 },
                         { "master", "MASTER", 0, 960, 0 }, { "boost", "BOOST", 1, 1088, 20 } };
            case 5:   // TimmyC: SVT limpio + SVT sucio
                return { { "gain", "GAIN", 0, 72, 0 }, { "ulo", "ULTRA LO", 1, 216, 0 }, { "uhi", "ULTRA HI", 1, 216, 46 },
                         { "bass", "BASS", 0, 360, 0 }, { "mid", "MID", 0, 504, 0 }, { "freq", "FREQ", 0, 648, 0 },
                         { "treble", "TREBLE", 0, 792, 0 }, { "dirty", "DIRTY MIX", 0, 936, 0 }, { "master", "MASTER", 0, 1080, 0 } };
            default:  // Ryandemar
                return { { "gain", "GAIN", 0, 72, 0 }, { "ulo", "ULTRA LO", 0, 216, 0 }, { "bass", "BASS", 0, 360, 0 }, { "treble", "TREBLE", 0, 504, 0 },
                         { "mid", "MID", 0, 648, 0 }, { "freq", "FREQ", 0, 792, 0 }, { "uhi", "ULTRA HI", 0, 936, 0 }, { "master", "MASTER", 0, 1080, 0 } };
        }
    }

    void layoutAmp()
    {
        for (auto& kv : ampKnobs) kv.second->setVisible (false);
        for (auto* c : { (juce::Component*) &boostBtn, (juce::Component*) &brightBtn, (juce::Component*) uloTog.get(), (juce::Component*) uhiTog.get() })
            c->setVisible (false);
        for (auto& s : eqSl) s.setVisible (false);

        const int y0 = ampY + 140;
        for (const auto& it : ampLayout (skin))
        {
            const int cx = 24 + (int) it.cx;
            const juce::String pid (it.pid);
            if (it.kind == 0)
            {
                auto* u = ampKnobs[pid];
                u->setName (it.label);
                u->setBounds (cx - 50, y0 + 10, 100, u->totalHeight());
                u->setVisible (true);
            }
            else if (it.kind == 1)
            {
                juce::TextButton* b = pid == "boost" ? &boostBtn : pid == "bright" ? &brightBtn : pid == "ulo" ? (juce::TextButton*) uloTog.get() : (juce::TextButton*) uhiTog.get();
                b->setBounds (cx - 48, y0 + 22 + (int) it.dy, 96, 32);
                b->setVisible (true);
            }
            else
            {
                const int idx = pid.getTrailingIntValue() - 1;
                eqSl[(size_t) idx].setBounds (cx - 16, y0 + 24, 32, 100);
                eqSl[(size_t) idx].setVisible (true);
            }
        }
    }

    void layoutModules (bool animate, int skipId)
    {
        for (int k = 0; k < fxN; ++k)
        {
            const int id = order[k];
            const juce::Rectangle<int> r (rowX0 + k * (fxW + fxGap), rowY, fxW, fxH);
            if (id == skipId) continue;
            if (animate) animator.animateComponent (byId[id], r, 1.0f, 140, false, 1.0, 1.0);
            else byId[id]->setBounds (r);
        }
    }

    void dragStart (int id, ModuleBox& b, const juce::MouseEvent& e)
    {
        dragId = id;
        dragOffset = e.getEventRelativeTo (this).x - b.getX();
        animator.cancelAllAnimations (false);
        b.toFront (false);
    }
    void dragMove (int id, ModuleBox& b, const juce::MouseEvent& e)
    {
        const int nx = juce::jlimit (rowX0, rowX0 + (fxN - 1) * (fxW + fxGap), e.getEventRelativeTo (this).x - dragOffset);
        b.setTopLeftPosition (nx, rowY);
        const int slot = juce::jlimit (0, fxN - 1, (nx + fxW / 2 - rowX0) / (fxW + fxGap));
        int cur = 0;
        while (cur < fxN && order[cur] != id) ++cur;
        if (cur < fxN && slot != cur)
        {
            const int step = slot > cur ? 1 : -1;
            for (int k = cur; k != slot; k += step) order[k] = order[k + step];
            order[slot] = id;
            layoutModules (true, id);
        }
    }
    void dragEnd (int, ModuleBox&, const juce::MouseEvent&)
    {
        dragId = -1;
        layoutModules (true, -1);
        orderAtt->setValueAsCompleteGesture ((float) jah::encodeOrder (order));
    }

    // Boton ON/OFF de un efecto: atenua las perillas cuando esta apagado
    void setupPower (ModuleBox& box, juce::TextButton& btn, const char* pid, juce::Colour col,
                     const std::vector<KnobUnit*>& ks, juce::Component* extra)
    {
        btn.setClickingTogglesState (true);
        btn.setColour (juce::TextButton::buttonOnColourId, col);
        btn.getProperties().set ("small", true);
        btn.setBounds (fxW - 8 - 44, 8, 44, 22);
        auto refresh = [&btn, ks, extra] ()
        {
            const bool on = btn.getToggleState();
            btn.setButtonText (on ? "ON" : "OFF");
            for (auto* k : ks) k->setActive (on);
            if (extra != nullptr) extra->setEnabled (on);
        };
        btn.onStateChange = refresh;
        box.addAndMakeVisible (btn);
        powerAtts.push_back (std::make_unique<APVTS::ButtonAttachment> (proc.apvts, pid, btn));
        refresh();
    }

    void stepPreset (int d)
    {
        const int n = jah::presetCount();
        const int cur = proc.getCurrentProgram();
        loadPreset (((cur + d) % n + n) % n);
    }
    void loadPreset (int i)
    {
        proc.loadPreset (i);
        lastPreset = i;
        presetBtn.setButtonText (jah::presetName (i));
    }
    void showPresetMenu()
    {
        juce::PopupMenu m;
        const char* groups[] = { "", "Pulga", "NotFieldy", "RoAguMiSSTruVe", "Ryandemar", "Canciller", "TimmyC", "Especiales" };
        for (int i = 0; i < jah::presetCount(); ++i)
        {
            if (i == 0 || i == 1 || i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19)
            {
                const int g = i == 0 ? 0 : (i >= 19 ? 7 : (i + 2) / 3);
                if (i > 0) m.addSeparator();
                if (g > 0) m.addSectionHeader (groups[g]);
            }
            m.addItem (i + 1, jah::presetName (i), true, i == proc.getCurrentProgram());
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (240),
                         [this] (int r) { if (r > 0) loadPreset (r - 1); });
    }
    void timerCallback() override
    {
        const int cur = proc.getCurrentProgram();
        if (cur != lastPreset) { lastPreset = cur; presetBtn.setButtonText (jah::presetName (cur)); }

        auto toDb = [] (float lin) { return lin > 1e-5f ? 20.0f * std::log10 (lin) : -90.0f; };
        inDb = toDb (proc.meterIn.load()); outDb = toDb (proc.meterOut.load());
        holdIn = std::max (inDb, holdIn - 0.6f); holdOut = std::max (outDb, holdOut - 0.6f);
        const float g = proc.meterGr.load();
        if (std::abs (g - grDb) > 0.05f) { grDb = g; dynBox->repaint (440, 30, 170, 30); }
        repaint (meterArea);
    }

    // Medidor de pico en segmentos: verde, amarillo (desde -18 dB) y rojo (desde -6 dB), con marca de pico
    void drawMeter (juce::Graphics& g, const juce::String& label, int x, int y, int w, int h, float db, float hold)
    {
        g.setColour (juce::Colour (0xff868c96));
        g.setFont (fnt (10.5f, juce::Font::bold));
        g.drawText (label, x - 34, y - 2, 30, h + 4, juce::Justification::centredRight);
        const int n = 48;
        const float sw = (float) w / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const float segDb = -60.0f + 60.0f * (float) i / (float) (n - 1);
            const juce::Colour c = segDb < -18.0f ? juce::Colour (0xff3fd27a) : (segDb < -6.0f ? juce::Colour (0xfff0c040) : juce::Colour (0xfff04848));
            const bool on = segDb <= db;
            g.setColour (on ? c : c.withAlpha (0.13f));
            g.fillRoundedRectangle ((float) x + (float) i * sw, (float) y, sw - 1.5f, (float) h, 1.5f);
        }
        const float hx = (float) x + juce::jlimit (0.0f, 1.0f, (hold + 60.0f) / 60.0f) * (float) (w - 2);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRect (hx, (float) y - 1.0f, 2.0f, (float) h + 2.0f);
        g.setColour (db > -0.5f ? juce::Colour (0xfff04848) : juce::Colour (0xff868c96));
        g.setFont (fnt (10.0f));
        g.drawText (db <= -89.0f ? juce::String ("-inf") : juce::String (db, 1) + " dB", x + w + 6, y - 2, 56, h + 4, juce::Justification::centredLeft);
    }

    void tap()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (! taps.empty() && now - taps.back() > 2000.0) taps.clear();
        taps.push_back (now);
        if (taps.size() > 5) taps.erase (taps.begin());
        if (taps.size() < 2) return;
        const double ms = (taps.back() - taps.front()) / (double) (taps.size() - 1);
        if (auto* prm = proc.apvts.getParameter ("dtime"))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (40.0, 1200.0, ms)));
            prm->endChangeGesture();
        }
    }

    KnobUnit* addKnob (juce::Component& parent, const juce::String& pid, const juce::String& name, int x, int y, int w, int size, bool amp,
                       juce::Colour col, std::function<juce::String (double)> fmt)
    {
        auto u = std::make_unique<KnobUnit> (proc.apvts, pid, name, size, amp, col, std::move (fmt));
        u->setBounds (x, y, w, u->totalHeight());
        parent.addAndMakeVisible (*u);
        auto* raw = u.get();
        knobs.push_back (std::move (u));
        return raw;
    }

    JustAnAmpProcessor& proc;
    JAHeadLookAndFeel& laf;
    std::function<void (float)> setScale;
    int skin = 0, pedalIdx = 0, lastPreset = 0, dragId = -1, dragOffset = 0;
    float inDb = -90.0f, outDb = -90.0f, holdIn = -90.0f, holdOut = -90.0f, grDb = 0.0f;
    const juce::Rectangle<int> meterArea { 396, 4, 440, 46 };
    int order[7] = { 0, 1, 2, 3, 4, 5, 6 };
    bool initialised = false;
    juce::Image heads[6];
    std::unique_ptr<ChoiceStrip> headStrip, pedStrip, compStrip;
    std::unique_ptr<ModuleBox> dynBox, pedBox;
    std::unique_ptr<ModuleBox> fxBox[7];
    std::vector<KnobUnit*> fxKnobs[7];
    juce::TextButton fxOn[7];
    ModuleBox* byId[7] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    std::vector<std::unique_ptr<KnobUnit>> knobs;
    std::map<juce::String, KnobUnit*> ampKnobs;
    std::vector<KnobUnit*> compKnobs, pedKnobs;
    juce::TextButton tapButton, prevBtn, nextBtn, presetBtn, boostBtn, brightBtn;
    std::unique_ptr<FloatToggle> uloTog, uhiTog;
    std::unique_ptr<APVTS::ButtonAttachment> boostAtt, brightAtt;
    std::array<juce::Slider, 7> eqSl;
    std::vector<std::unique_ptr<APVTS::SliderAttachment>> eqAtt;
    std::vector<std::unique_ptr<APVTS::ButtonAttachment>> powerAtts;
    std::unique_ptr<juce::ParameterAttachment> orderAtt;
    juce::ComponentAnimator animator;
    std::vector<double> taps;
    std::array<juce::TextButton, 4> scaleButtons;
};

JustAnAmpEditor::JustAnAmpEditor (JustAnAmpProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    // Leer el tamaño guardado antes de nada: setResizeLimits() dispara resized() y lo pisaría.
    const int savedW = juce::jlimit (600, 1800, (int) proc.apvts.state.getProperty ("uiWidth", designW));
    content = std::make_unique<Content> (p, laf, [this] (float s)
    {
        setSize (juce::roundToInt ((float) designW * s), juce::roundToInt ((float) designH * s));
    });
    content->setLookAndFeel (&laf);
    addAndMakeVisible (*content);

    setResizeLimits (600, 423, 1800, 1269);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);

    setSize (savedW, juce::roundToInt ((double) savedW * designH / designW));
}

JustAnAmpEditor::~JustAnAmpEditor()
{
    content->setLookAndFeel (nullptr);
}

void JustAnAmpEditor::resized()
{
    const float s = (float) getWidth() / (float) designW;
    content->setBounds (0, 0, designW, designH);
    content->setTransform (juce::AffineTransform::scale (s));
    content->updateScaleHighlight (s);
    proc.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}
