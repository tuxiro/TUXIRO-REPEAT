#pragma once
#include "PluginProcessor.h"

// Diseno en un espacio de 1260 x 915 px, escalado al tamano real de la ventana.
namespace tx
{
inline const juce::Colour kGray      { 0xffc3c7c9 };
inline const juce::Colour kGrayLight { 0xffd6d9db };
inline const juce::Colour kGrayMid   { 0xffb4b9bb };
inline const juce::Colour kGrayDark  { 0xff5b6063 };
inline const juce::Colour kGreen     { 0xff0b7a2d };
inline const juce::Colour kGreenDeep { 0xff043a12 };
inline const juce::Colour kGreenText { 0xff0a4d1c };
inline const juce::Colour kGrid      { 0xff0e5a22 };
inline const juce::Colour kLime      { 0xff2ee05a };
inline const juce::Colour kYellow    { 0xffffe600 };

inline juce::Font monoFont (float h)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::bold));
}

// Texto en mayusculas con espaciado entre letras (estilo del plugin original)
inline void drawSpaced (juce::Graphics& g, const juce::String& s, float x, float yCentre, float size,
                        juce::Colour c, float spacing = 0.0f, float centreWidth = 0.0f)
{
    g.setFont (monoFont (size));
    g.setColour (c);
    const float adv = size * 0.6f + spacing;
    const float total = adv * (float) s.length() - spacing;
    const float x0 = centreWidth > 0.0f ? x + (centreWidth - total) * 0.5f : x;
    for (int i = 0; i < s.length(); ++i)
        g.drawText (juce::String::charToString (s[i]),
                    juce::Rectangle<float> (x0 + (float) i * adv - 1.0f, yCentre - size, adv + 2.0f, size * 2.0f),
                    juce::Justification::centred, false);
}

inline const char* const* glyph (char c)
{
    static const char* gT[7] = { "11111","00100","00100","00100","00100","00100","00100" };
    static const char* gU[7] = { "10001","10001","10001","10001","10001","10001","01110" };
    static const char* gX[7] = { "10001","10001","01010","00100","01010","10001","10001" };
    static const char* gI[7] = { "01110","00100","00100","00100","00100","00100","01110" };
    static const char* gR[7] = { "11110","10001","10001","11110","10100","10010","10001" };
    static const char* gO[7] = { "01110","10001","10001","10001","10001","10001","01110" };
    static const char* gE[7] = { "11111","10000","10000","11110","10000","10000","11111" };
    static const char* gP[7] = { "11110","10001","10001","11110","10000","10000","10000" };
    static const char* gA[7] = { "01110","10001","10001","11111","10001","10001","10001" };
    switch (c)
    {
        case 'T': return gT;  case 'U': return gU;  case 'X': return gX;
        case 'I': return gI;  case 'R': return gR;  case 'O': return gO;
        case 'E': return gE;  case 'P': return gP;  case 'A': return gA;
        default:  return nullptr;
    }
}

inline void drawPixelText (juce::Graphics& g, const juce::String& s, float x, float y, float pw, float ph, juce::Colour c)
{
    g.setColour (c);
    for (int k = 0; k < s.length(); ++k)
    {
        if (auto* gl = glyph ((char) s[k]))
            for (int r = 0; r < 7; ++r)
                for (int col = 0; col < 5; ++col)
                    if (gl[r][col] == '1')
                        g.fillRect (x + (float) col * pw, y + (float) r * ph, pw + 0.6f, ph + 0.6f);
        x += 6.0f * pw;
    }
}

inline void drawWaveIcon (juce::Graphics& g, juce::Rectangle<float> r, int shape, juce::Colour c)
{
    juce::Path p;
    auto pt = [&] (float x, float y) { return juce::Point<float> (r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight()); };
    if (shape == 1)
    {
        for (int i = 0; i <= 40; ++i)
        {
            const float t = (float) i / 40.0f;
            auto q = pt (t, 0.5f - 0.4f * std::sin (juce::MathConstants<float>::twoPi * t));
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    }
    else if (shape == 2)
    {
        p.startNewSubPath (pt (0.0f, 0.9f)); p.lineTo (pt (0.75f, 0.1f));
        p.lineTo (pt (0.75f, 0.9f)); p.lineTo (pt (1.0f, 0.9f));
    }
    else if (shape == 3)
    {
        p.startNewSubPath (pt (0.0f, 0.9f)); p.lineTo (pt (0.3f, 0.9f)); p.lineTo (pt (0.3f, 0.1f));
        p.lineTo (pt (0.55f, 0.1f)); p.lineTo (pt (0.55f, 0.9f)); p.lineTo (pt (1.0f, 0.9f));
    }
    else
    {
        p.startNewSubPath (pt (0.0f, 0.9f)); p.lineTo (pt (0.0f, 0.1f)); p.lineTo (pt (0.5f, 0.1f));
        p.lineTo (pt (0.5f, 0.9f)); p.lineTo (pt (1.0f, 0.9f)); p.lineTo (pt (1.0f, 0.1f));
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (2.6f));
}

inline void drawCurveIcon (juce::Graphics& g, juce::Rectangle<float> r, int type, juce::Colour c)
{
    juce::Path p;
    for (int i = 0; i <= 32; ++i)
    {
        const float t = (float) i / 32.0f;
        const float v = trx::curveShape (type, t);
        const juce::Point<float> q (r.getX() + t * r.getWidth(), r.getBottom() - v * r.getHeight());
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (2.2f));
}
} // namespace tx

//==============================================================================
class RetroLNF : public juce::LookAndFeel_V4
{
public:
    RetroLNF()
    {
        setColour (juce::ComboBox::textColourId, tx::kGreenText);
        setColour (juce::PopupMenu::backgroundColourId, tx::kGrayLight);
        setColour (juce::PopupMenu::textColourId, tx::kGreenText);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, tx::kGreen);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return tx::monoFont (19.0f); }
    juce::Font getPopupMenuFont() override { return tx::monoFont (18.0f); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        const float fw = (float) w, fh = (float) h;
        g.setColour (tx::kGrayLight);
        g.fillRect (0.0f, 0.0f, fw, fh);
        g.setColour (tx::kGrayDark);
        g.fillRect (0.0f, 0.0f, fw, 2.0f);
        g.fillRect (0.0f, 0.0f, 2.0f, fh);
        g.setColour (juce::Colours::white);
        g.fillRect (0.0f, fh - 2.0f, fw, 2.0f);
        g.fillRect (fw - 2.0f, 0.0f, 2.0f, fh);
        juce::Path arrow;
        const float cx = fw - 22.0f, cy = fh * 0.5f;
        arrow.addTriangle (cx - 7.0f, cy - 4.0f, cx + 7.0f, cy - 4.0f, cx, cy + 5.0f);
        g.setColour (tx::kGreenText);
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 2, box.getWidth() - 40, box.getHeight() - 4);
        label.setFont (getComboBoxFont (box));
    }

    int getSliderThumbRadius (juce::Slider&) override { return 11; }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            const float cx = (float) x + (float) w * 0.5f;
            g.setColour (tx::kGrayDark);
            for (int i = 0; i <= 10; ++i)
            {
                const float yy = (float) y + 11.0f + ((float) h - 22.0f) * (float) i / 10.0f;
                g.fillRect (cx - 18.0f, yy, 8.0f, 1.5f);
                g.fillRect (cx + 10.0f, yy, 8.0f, 1.5f);
            }
            g.setColour (juce::Colours::black);
            g.fillRect (cx - 2.0f, (float) y + 4.0f, 4.0f, (float) h - 8.0f);
            const juce::Rectangle<float> t (cx - 17.0f, pos - 10.0f, 34.0f, 20.0f);
            g.setColour (tx::kYellow);
            g.fillRect (t);
            g.setColour (juce::Colours::black);
            g.drawRect (t, 2.0f);
            g.fillRect (t.getX() + 5.0f, t.getCentreY() - 1.0f, t.getWidth() - 10.0f, 2.0f);
        }
        else
        {
            const float cy = (float) y + (float) h * 0.5f;
            g.setColour (juce::Colours::black);
            g.fillRect ((float) x + 4.0f, cy - 2.0f, (float) w - 8.0f, 4.0f);
            g.setColour (tx::kYellow);
            g.fillRect ((float) x + 4.0f, cy - 1.5f, juce::jmax (0.0f, pos - (float) x - 4.0f), 3.0f);
            const juce::Rectangle<float> t (pos - 8.0f, cy - 15.0f, 16.0f, 30.0f);
            g.setColour (tx::kYellow);
            g.fillRect (t);
            g.setColour (juce::Colours::black);
            g.drawRect (t, 2.0f);
        }
    }
};

//==============================================================================
class RetroButton : public juce::Button
{
public:
    explicit RetroButton (const juce::String& t = {}) : juce::Button (t), label (t) {}

    juce::String label;
    float fontSize = 18.0f;
    bool alwaysYellow = false;
    std::function<void (juce::Graphics&, juce::Rectangle<float>, bool)> icon;

    void paintButton (juce::Graphics& g, bool, bool down) override
    {
        const auto b = getLocalBounds().toFloat();
        const bool on = getToggleState() || alwaysYellow;
        g.setColour (on ? tx::kYellow : (down ? tx::kGrayMid : juce::Colour (0xffcfd3d5)));
        g.fillRect (b);
        if (on)
        {
            g.setColour (juce::Colours::black);
            g.drawRect (b, 2.0f);
        }
        else
        {
            g.setColour (juce::Colours::white);
            g.fillRect (b.getX(), b.getY(), b.getWidth(), 2.0f);
            g.fillRect (b.getX(), b.getY(), 2.0f, b.getHeight());
            g.setColour (tx::kGrayDark);
            g.fillRect (b.getX(), b.getBottom() - 2.0f, b.getWidth(), 2.0f);
            g.fillRect (b.getRight() - 2.0f, b.getY(), 2.0f, b.getHeight());
        }

        const auto ink = on ? juce::Colours::black : tx::kGreenText;
        auto area = b.reduced (4.0f);
        if (icon && label.isNotEmpty())
        {
            auto ia = area.removeFromLeft (area.getHeight() + 6.0f);
            icon (g, ia.reduced (8.0f), on);
            tx::drawSpaced (g, label, area.getX(), area.getCentreY(), fontSize, ink, 3.0f, area.getWidth());
        }
        else if (icon)
            icon (g, area.reduced (6.0f), on);
        else
            tx::drawSpaced (g, label, area.getX(), area.getCentreY(), fontSize, ink, 3.0f, area.getWidth());
    }
};

// Boton de exportar audio: arrastrar a Ableton = audio WAV; click = guardar en el Escritorio
class DragButton : public RetroButton
{
public:
    using RetroButton::RetroButton;
    std::function<void()> onDragOut;

    void mouseDown (const juce::MouseEvent& e) override { dragged = false; RetroButton::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        RetroButton::mouseDrag (e);
        if (! dragged && e.getDistanceFromDragStart() > 6)
        {
            dragged = true;
            if (onDragOut) onDragOut();
            setState (juce::Button::buttonNormal);
        }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragged) { dragged = false; setState (juce::Button::buttonNormal); return; }
        RetroButton::mouseUp (e);
    }
private:
    bool dragged = false;
};

class TabButton : public juce::Button
{
public:
    explicit TabButton (const juce::String& t) : juce::Button (t), label (t) {}
    juce::String label;
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const auto b = getLocalBounds().toFloat();
        const bool on = getToggleState();
        g.setColour (on ? tx::kGrayLight : tx::kGrayMid);
        g.fillRect (b);
        g.setColour (on ? juce::Colours::white : tx::kGrayDark);
        g.drawRect (b, 1.5f);
        const juce::Rectangle<float> sq (b.getX() + 14.0f, b.getCentreY() - 12.0f, 24.0f, 24.0f);
        g.setColour (tx::kYellow);
        g.fillRect (sq);
        g.setColour (juce::Colours::black);
        g.drawRect (sq, 2.0f);
        tx::drawSpaced (g, label, b.getX() + 52.0f, b.getCentreY(), 17.0f, tx::kGreenText, 6.0f);
    }
};

class ProgressMeter : public juce::Component
{
public:
    float value = 0.0f;
    void paint (juce::Graphics& g) override
    {
        g.fillAll (tx::kGreenDeep);
        g.setColour (tx::kGrayDark);
        g.drawRect (getLocalBounds(), 2);
        const int n = 36;
        const float w = ((float) getWidth() - 10.0f) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            g.setColour ((float) i < value * (float) n ? tx::kLime : tx::kGrid);
            g.fillRect (5.0f + (float) i * w + 1.0f, 7.0f, w - 2.0f, (float) getHeight() - 14.0f);
        }
    }
};

//==============================================================================
class CurveEditor : public juce::Component
{
public:
    explicit CurveEditor (TuxiroProcessor& p) : proc (p) {}
    int lane = 0;   // 0 rate, 1 pitch, 2 noise

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> inner() const { return { 19.0f, 16.0f, 906.0f, 222.0f }; }
    float raw (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    float norm (const char* id) const;
    float riseFrac() const;
    juce::String describe (const char* id) const;

    TuxiroProcessor& proc;
    int dragging = -1;
};

//==============================================================================
class TuxiroContent : public juce::Component, private juce::Timer
{
public:
    explicit TuxiroContent (TuxiroProcessor&);
    ~TuxiroContent() override { stopTimer(); }
    void paint (juce::Graphics&) override;

private:
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void setLane (int);
    void initCombo (juce::ComboBox&, const char* id, juce::Rectangle<int> r);
    void initSlider (juce::Slider&, const char* id, juce::Slider::SliderStyle, juce::Rectangle<int> r);
    const char* curveId() const { return lane == 0 ? "rateCurve" : lane == 1 ? "pitchCurve" : "noiseCurve"; }

    TuxiroProcessor& proc;
    int lane = 0;
    juce::String status = "READY.";
    int statusHold = 0;

    CurveEditor curve;
    TabButton tabs[3] { TabButton ("RATE"), TabButton ("PITCH"), TabButton ("NOISE") };
    RetroButton curveBtns[8], shapeBtns[4], lenBtns[7];
    RetroButton prevBtn, nextBtn, playBtn, randBtn, undoBtn, syncBtn;
    DragButton dragBtn;
    juce::ComboBox presetBox, modeBox, startBox, endBox, gapBox, tailBox, trigBox;
    juce::Slider mixSl[4], zapDepthSl, zapLenSl;
    juce::Label zapDepthLbl, zapLenLbl;
    ProgressMeter meter;

    std::vector<std::unique_ptr<CA>> comboAtt;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;
};

class TuxiroEditor : public juce::AudioProcessorEditor
{
public:
    explicit TuxiroEditor (TuxiroProcessor&);
    ~TuxiroEditor() override { setLookAndFeel (nullptr); }
    void resized() override;

private:
    RetroLNF lnf;
    TuxiroContent content;
};
