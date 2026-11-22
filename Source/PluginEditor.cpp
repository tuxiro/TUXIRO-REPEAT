#include "PluginEditor.h"

using namespace tx;

namespace
{
    struct LaneInfo { const char* s; const char* e; float lo, hi; const char* curve; };
    const LaneInfo kLanes[3] = {
        { "startRate", "endRate", 0.0f, 6.0f,  "rateCurve"  },
        { "pStart",    "pEnd",    0.0f, 60.0f, "pitchCurve" },
        { "nStart",    "nEnd",    0.0f, 1.0f,  "noiseCurve" }
    };
    const char* const kRateNames[7] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
}

//==============================================================================
// CurveEditor
//==============================================================================
float CurveEditor::norm (const char* id) const
{
    const auto& L = kLanes[lane];
    return juce::jlimit (0.0f, 1.0f, (raw (id) - L.lo) / (L.hi - L.lo));
}

float CurveEditor::riseFrac() const
{
    const int li = juce::jlimit (0, 6, (int) std::lround (raw ("length")));
    const int gi = juce::jlimit (0, 5, (int) std::lround (raw ("endGap")));
    const double total = trx::kLengthBars[li] * 4.0;
    const double gap = std::min<double> (trx::kGapBeats[gi], total * 0.5);
    return (float) ((total - gap) / total);
}

juce::String CurveEditor::describe (const char* id) const
{
    const float v = raw (id);
    if (lane == 0) return kRateNames[juce::jlimit (0, 6, (int) std::lround (v))];
    if (lane == 1) return "+" + juce::String ((int) std::lround (v)) + " ST";
    return juce::String ((int) std::lround (v * 100.0f)) + "%";
}

void CurveEditor::paint (juce::Graphics& g)
{
    const auto in = inner();
    g.fillAll (kGreenDeep);

    const int bars = trx::kLengthBars[juce::jlimit (0, 6, (int) std::lround (raw ("length")))];
    const int div = juce::jmin (bars, 8);

    // rejilla
    g.setColour (kGrid);
    for (int i = 0; i <= div; ++i)
    {
        const float x = in.getX() + in.getWidth() * (float) i / (float) div;
        g.fillRect (x - 0.6f, in.getY() - 4.0f, 1.2f, in.getHeight() + 8.0f);
    }
    for (int j = 0; j <= 4; ++j)
    {
        const float y = in.getY() + in.getHeight() * (float) j / 4.0f;
        g.fillRect (in.getX(), y - 0.6f, in.getWidth(), 1.2f);
    }

    // numeros de compas + DROP
    for (int i = 0; i < div; ++i)
    {
        const float x = in.getX() + in.getWidth() * (float) i / (float) div;
        drawSpaced (g, juce::String (1 + i * bars / div), x - 20.0f, in.getBottom() + 13.0f, 15.0f,
                    juce::Colours::white.withAlpha (0.85f), 0.0f, 40.0f);
    }
    drawSpaced (g, "DROP", in.getRight() - 30.0f, in.getBottom() + 13.0f, 15.0f, kYellow, 1.0f, 60.0f);

    // zona del silencio previo al drop (end gap)
    const float frac = riseFrac();
    const float xg = in.getX() + in.getWidth() * frac;
    if (xg < in.getRight() - 1.0f)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<int> ((int) xg, (int) in.getY() - 4, (int) (in.getRight() - xg) + 1, (int) in.getHeight() + 8));
        g.setColour (kGrid);
        for (float x = xg - in.getHeight(); x < in.getRight(); x += 7.0f)
            g.drawLine (x, in.getBottom(), x + in.getHeight(), in.getY(), 1.2f);
    }

    // curva
    const auto& L = kLanes[lane];
    const float s = norm (L.s), e = norm (L.e);
    const int type = juce::jlimit (0, 7, (int) std::lround (raw (L.curve)));
    const float wCurve = in.getWidth() * frac;

    juce::Path line, fill;
    for (int k = 0; k <= 160; ++k)
    {
        const float t = (float) k / 160.0f;
        const float v = s + (e - s) * trx::curveShape (type, t);
        const juce::Point<float> q (in.getX() + wCurve * t, in.getBottom() - in.getHeight() * v);
        if (k == 0) { line.startNewSubPath (q); fill.startNewSubPath (in.getX(), in.getBottom()); fill.lineTo (q); }
        else        { line.lineTo (q); fill.lineTo (q); }
    }
    fill.lineTo (in.getX() + wCurve, in.getBottom());
    fill.closeSubPath();
    g.setColour (kYellow.withAlpha (0.14f));
    g.fillPath (fill);
    g.setColour (kYellow);
    g.strokePath (line, juce::PathStrokeType (3.0f));

    // etiquetas START / END
    drawSpaced (g, "END   " + describe (L.e), in.getX() + 12.0f, in.getY() + 16.0f, 15.0f,
                juce::Colours::white.withAlpha (0.7f), 1.0f);
    drawSpaced (g, "START " + describe (L.s), in.getX() + 12.0f, in.getBottom() - 30.0f, 15.0f,
                juce::Colours::white.withAlpha (0.7f), 1.0f);

    // cabeza de reproduccion
    const float px = in.getX() + in.getWidth() * proc.uiFraction.load();
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillRect (px - 1.2f, in.getY() - 6.0f, 2.4f, in.getHeight() + 12.0f);

    // tiradores
    auto handle = [&] (juce::Point<float> c)
    {
        const juce::Rectangle<float> r (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (kYellow);
        g.fillRect (r);
        g.setColour (juce::Colours::black);
        g.drawRect (r, 2.0f);
    };
    handle ({ in.getX(), in.getBottom() - in.getHeight() * s });
    handle ({ in.getX() + wCurve, in.getBottom() - in.getHeight() * e });
}

void CurveEditor::mouseDown (const juce::MouseEvent& ev)
{
    const auto in = inner();
    const auto& L = kLanes[lane];
    const float frac = riseFrac();
    const juce::Point<float> hs (in.getX(), in.getBottom() - in.getHeight() * norm (L.s));
    const juce::Point<float> he (in.getX() + in.getWidth() * frac, in.getBottom() - in.getHeight() * norm (L.e));
    const auto p = ev.position;

    dragging = -1;
    const float ds = p.getDistanceFrom (hs), de = p.getDistanceFrom (he);
    if (ds < 20.0f || de < 20.0f)
        dragging = ds <= de ? 0 : 1;

    if (dragging >= 0)
        if (auto* prm = proc.apvts.getParameter (dragging == 0 ? L.s : L.e))
            prm->beginChangeGesture();
}

void CurveEditor::mouseDrag (const juce::MouseEvent& ev)
{
    if (dragging < 0) return;
    const auto in = inner();
    const auto& L = kLanes[lane];
    const float n = juce::jlimit (0.0f, 1.0f, (in.getBottom() - ev.position.y) / in.getHeight());
    float plain = L.lo + n * (L.hi - L.lo);
    if (lane == 0) plain = std::round (plain);
    if (auto* prm = proc.apvts.getParameter (dragging == 0 ? L.s : L.e))
        prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
    repaint();
}

void CurveEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0)
        if (auto* prm = proc.apvts.getParameter (dragging == 0 ? kLanes[lane].s : kLanes[lane].e))
            prm->endChangeGesture();
    dragging = -1;
}

//==============================================================================
// TuxiroContent
//==============================================================================
void TuxiroContent::initCombo (juce::ComboBox& cb, const char* id, juce::Rectangle<int> r)
{
    if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        cb.addItemList (pc->choices, 1);
    addAndMakeVisible (cb);
    cb.setBounds (r);
    comboAtt.push_back (std::make_unique<CA> (proc.apvts, id, cb));
}

void TuxiroContent::initSlider (juce::Slider& sl, const char* id, juce::Slider::SliderStyle st, juce::Rectangle<int> r)
{
    sl.setSliderStyle (st);
    sl.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (sl);
    sl.setBounds (r);
    sliderAtt.push_back (std::make_unique<SA> (proc.apvts, id, sl));
}

TuxiroContent::TuxiroContent (TuxiroProcessor& p) : proc (p), curve (p)
{
    setSize (1260, 915);

    addAndMakeVisible (curve);
    curve.setBounds (27, 241, 928, 272);

    // pestanas RATE / PITCH / NOISE
    const int tabX[3] = { 20, 226, 430 };
    const int tabW[3] = { 205, 203, 203 };
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible (tabs[i]);
        tabs[i].setBounds (tabX[i], 160, tabW[i], 41);
        tabs[i].onClick = [this, i] { setLane (i); };
    }

    // botones de forma de curva
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (curveBtns[i]);
        curveBtns[i].setBounds (606 + i * 42, 207, 38, 26);
        curveBtns[i].icon = [i] (juce::Graphics& g, juce::Rectangle<float> r, bool on)
        { drawCurveIcon (g, r, i, on ? juce::Colours::black : kGreenText); };
        curveBtns[i].onClick = [this, i] { proc.setParamPlain (curveId(), (float) i); };
    }

    // preset
    presetBox.addItemList (TuxiroProcessor::presetNames, 1);
    addAndMakeVisible (presetBox);
    presetBox.setBounds (757, 80, 283, 42);
    presetBox.setTextWhenNothingSelected ("Custom");
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0) proc.applyPreset (presetBox.getSelectedId() - 1);
    };

    auto stepPreset = [this] (int dir)
    {
        const int n = TuxiroProcessor::presetNames.size();
        int cur = proc.apvts.state.getProperty ("preset", 0);
        if (cur < 0) cur = 0;
        proc.applyPreset ((cur + dir + n) % n);
    };
    addAndMakeVisible (prevBtn);
    prevBtn.setBounds (1047, 80, 45, 42);
    prevBtn.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, bool on)
    {
        juce::Path t;
        t.addTriangle (r.getRight() - 4.0f, r.getY(), r.getRight() - 4.0f, r.getBottom(), r.getX() + 2.0f, r.getCentreY());
        g.setColour (on ? juce::Colours::black : kGreenText);
        g.fillPath (t);
    };
    prevBtn.onClick = [stepPreset] { stepPreset (-1); };
    addAndMakeVisible (nextBtn);
    nextBtn.setBounds (1098, 80, 45, 42);
    nextBtn.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, bool on)
    {
        juce::Path t;
        t.addTriangle (r.getX() + 4.0f, r.getY(), r.getX() + 4.0f, r.getBottom(), r.getRight() - 2.0f, r.getCentreY());
        g.setColour (on ? juce::Colours::black : kGreenText);
        g.fillPath (t);
    };
    nextBtn.onClick = [stepPreset] { stepPreset (1); };

    // fila RATE
    initCombo (modeBox,  "mode",      { 35, 598, 200, 35 });
    initCombo (startBox, "startRate", { 272, 598, 178, 35 });
    initCombo (endBox,   "endRate",   { 486, 598, 178, 35 });

    for (int i = 0; i < 4; ++i)
    {
        addAndMakeVisible (shapeBtns[i]);
        shapeBtns[i].setBounds (35 + i * 80, 675, 70, 36);
        shapeBtns[i].icon = [i] (juce::Graphics& g, juce::Rectangle<float> r, bool on)
        { drawWaveIcon (g, r, i, on ? juce::Colours::black : kGreenText); };
        shapeBtns[i].onClick = [this, i] { proc.setParamPlain ("shape", (float) i); };
    }

    initSlider (zapDepthSl, "zapDepth", juce::Slider::LinearHorizontal, { 396, 675, 392, 36 });
    initSlider (zapLenSl,   "zapLen",   juce::Slider::LinearHorizontal, { 835, 675, 372, 36 });
    for (auto* l : { &zapDepthLbl, &zapLenLbl })
    {
        addAndMakeVisible (*l);
        l->setFont (monoFont (16.0f));
        l->setColour (juce::Label::textColourId, kGreenText);
        l->setJustificationType (juce::Justification::centredRight);
        l->setInterceptsMouseClicks (false, false);
    }
    zapDepthLbl.setBounds (680, 650, 110, 24);
    zapLenLbl.setBounds (1100, 650, 110, 24);

    // layer mix
    const char* mixIds[4] = { "mixRate", "mixPitch", "mixNoise", "mixFx" };
    const int mixX[4] = { 1031, 1087, 1144, 1200 };
    for (int i = 0; i < 4; ++i)
        initSlider (mixSl[i], mixIds[i], juce::Slider::LinearVertical, { mixX[i] - 22, 300, 44, 194 });

    // fila GLOBAL
    for (int i = 0; i < 7; ++i)
    {
        static const char* names[7] = { "1", "2", "4", "8", "16", "32", "64" };
        lenBtns[i].label = names[i];
        lenBtns[i].fontSize = 22.0f;
        addAndMakeVisible (lenBtns[i]);
        lenBtns[i].setBounds ((int) (35 + i * 75.5), 797, 66, 34);
        lenBtns[i].onClick = [this, i] { proc.setParamPlain ("length", (float) i); };
    }

    addAndMakeVisible (syncBtn);
    syncBtn.setBounds (580, 798, 32, 32);
    syncBtn.setClickingTogglesState (true);
    buttonAtt.push_back (std::make_unique<BA> (proc.apvts, "sync", syncBtn));

    initCombo (trigBox, "trigger", { 668, 800, 100, 32 });
    initCombo (gapBox,  "endGap",  { 785, 800, 190, 32 });
    initCombo (tailBox, "tail",    { 1000, 800, 225, 32 });

    // barra inferior
    addAndMakeVisible (meter);
    meter.setBounds (20, 857, 380, 40);

    addAndMakeVisible (playBtn);
    playBtn.setBounds (603, 853, 133, 48);
    playBtn.alwaysYellow = true;
    playBtn.label = "PLAY";
    playBtn.fontSize = 16.0f;
    playBtn.icon = [this] (juce::Graphics& g, juce::Rectangle<float> r, bool)
    {
        g.setColour (juce::Colours::black);
        if (proc.uiState.load() > 0)
            g.fillRect (r.reduced (3.0f));
        else
        {
            juce::Path t;
            t.addTriangle (r.getX(), r.getY(), r.getX(), r.getBottom(), r.getRight(), r.getCentreY());
            g.fillPath (t);
        }
    };
    playBtn.onClick = [this] { proc.uiCommand = proc.uiState.load() > 0 ? 2 : 1; };

    addAndMakeVisible (randBtn);
    randBtn.setBounds (747, 853, 165, 48);
    randBtn.label = "RANDOMIZE";
    randBtn.fontSize = 15.0f;
    randBtn.onClick = [this] { proc.randomize(); };

    addAndMakeVisible (undoBtn);
    undoBtn.setBounds (918, 853, 52, 48);
    undoBtn.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, bool)
    {
        juce::Path a;
        a.addCentredArc (r.getCentreX(), r.getCentreY() + 2.0f, 9.0f, 9.0f, 0.0f, 0.4f, 4.6f, true);
        g.setColour (kGreenText);
        g.strokePath (a, juce::PathStrokeType (2.6f));
        juce::Path h;
        h.addTriangle (r.getX() + 1.0f, r.getCentreY() - 6.0f, r.getX() + 13.0f, r.getCentreY() - 6.0f,
                       r.getX() + 7.0f, r.getCentreY() + 4.0f);
        g.fillPath (h);
    };
    undoBtn.onClick = [this] { proc.undo(); };

    addAndMakeVisible (dragBtn);
    dragBtn.setBounds (978, 853, 262, 48);
    dragBtn.label = "DRAG AUDIO";
    dragBtn.fontSize = 15.0f;
    dragBtn.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, bool)
    {
        g.setColour (kGreenText);
        const float cx = r.getCentreX();
        g.fillRect (cx - 1.5f, r.getY() - 2.0f, 3.0f, r.getHeight() - 8.0f);
        juce::Path t;
        t.addTriangle (cx - 7.0f, r.getBottom() - 12.0f, cx + 7.0f, r.getBottom() - 12.0f, cx, r.getBottom() - 3.0f);
        g.fillPath (t);
        g.fillRect (cx - 9.0f, r.getBottom() - 1.0f, 18.0f, 3.0f);
    };

    auto fileName = [this]
    {
        const auto prm = proc.readParams();
        return "TUXIRO_REPEAT_" + juce::String (prm.lengthBars) + "bars_" + juce::String ((int) proc.uiBpm.load()) + "bpm.wav";
    };
    dragBtn.onDragOut = [this, fileName]
    {
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (fileName());
        if (proc.renderToWav (f))
            juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray { f.getFullPathName() }, false, this);
    };
    dragBtn.onClick = [this, fileName]
    {
        const auto f = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (fileName());
        status = proc.renderToWav (f) ? "SAVED TO DESKTOP." : "SAVE FAILED.";
        statusHold = 90;
        repaint (400, 860, 190, 40);
    };

    setLane (0);
    startTimerHz (30);
}

void TuxiroContent::setLane (int i)
{
    lane = i;
    curve.lane = i;
    for (int k = 0; k < 3; ++k) tabs[k].setToggleState (k == i, juce::dontSendNotification);
    curve.repaint();
    repaint (22, 203, 600, 32);
}

void TuxiroContent::timerCallback()
{
    auto raw = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    auto idx = [&] (const char* id) { return (int) std::lround (raw (id)); };

    const int len = idx ("length");
    for (int i = 0; i < 7; ++i) lenBtns[i].setToggleState (i == len, juce::dontSendNotification);
    const int sh = idx ("shape");
    for (int i = 0; i < 4; ++i) shapeBtns[i].setToggleState (i == sh, juce::dontSendNotification);
    const int cv = idx (curveId());
    for (int i = 0; i < 8; ++i) curveBtns[i].setToggleState (i == cv, juce::dontSendNotification);

    zapDepthLbl.setText ("+" + juce::String ((int) std::lround (raw ("zapDepth"))) + " ST", juce::dontSendNotification);
    zapLenLbl.setText (juce::String ((int) std::lround (raw ("zapLen"))) + "%", juce::dontSendNotification);

    const int pi = proc.apvts.state.getProperty ("preset", -1);
    presetBox.setSelectedId (pi >= 0 ? pi + 1 : 0, juce::dontSendNotification);

    const int st = proc.uiState.load();
    const juce::String wantLabel = st > 0 ? "STOP" : "PLAY";
    if (playBtn.label != wantLabel) { playBtn.label = wantLabel; }
    playBtn.repaint();

    juce::String s = status;
    if (statusHold > 0) --statusHold;
    else s = st == 1 ? "BUILDING." : st == 2 ? "DROP!" : "READY.";
    if (s != status) { status = s; repaint (400, 860, 190, 40); }

    meter.value = proc.uiFraction.load();
    meter.repaint();
    curve.repaint();
}

void TuxiroContent::paint (juce::Graphics& g)
{
    g.fillAll (kGray);

    // ---- barra de titulo ----
    g.setColour (kGreen);
    g.fillRect (0, 0, 1260, 38);
    const float eq[4] = { 10.0f, 16.0f, 8.0f, 14.0f };
    g.setColour (kYellow);
    for (int i = 0; i < 4; ++i) g.fillRect (14.0f + (float) i * 5.0f, 28.0f - eq[i], 3.0f, eq[i]);
    drawSpaced (g, "TUXIRO - REPEAT", 48.0f, 19.0f, 14.0f, juce::Colours::white, 5.0f);
    g.setColour (kGrayLight);
    g.fillRect (1194, 8, 24, 22);
    g.fillRect (1224, 8, 24, 22);
    g.setColour (kGreenText);
    g.fillRect (1199, 23, 12, 3);
    g.drawRect (1230, 13, 12, 12, 2);

    // ---- logo ----
    drawPixelText (g, "TUXIRO", 38.0f, 52.0f, 2.6f, 2.6f, kGreenText);
    drawPixelText (g, "REPEAT", 40.0f, 84.0f, 12.0f, 8.0f, kGreen);
    drawSpaced (g, "RISER GENERATOR", 528.0f, 103.0f, 12.0f, kGreenText, 7.0f);
    g.setColour (kGreenText);
    g.fillRect (528, 119, 214, 2);

    // ---- smiley ----
    const juce::Rectangle<float> face (1167.0f, 55.0f, 95.0f, 80.0f);
    g.setColour (kYellow);
    g.fillEllipse (face);
    g.setColour (juce::Colours::black);
    g.drawEllipse (face, 3.0f);
    g.fillEllipse (1193.0f, 76.0f, 9.0f, 17.0f);
    g.fillEllipse (1227.0f, 76.0f, 9.0f, 17.0f);
    juce::Path smile;
    smile.addCentredArc (1214.5f, 93.0f, 25.0f, 20.0f, 0.0f, 2.3f, 3.98f, true);
    g.strokePath (smile, juce::PathStrokeType (3.5f));

    // ---- panel de curva ----
    g.setColour (kGrayLight);
    g.fillRect (22, 203, 953, 317);
    g.setColour (kGrayDark);
    g.drawRect (22, 203, 953, 317, 2);
    g.setColour (kGreen);
    g.fillRect (22, 203, 953, 32);
    static const char* titles[3] = { "ACCELERATION CURVE", "PITCH CURVE", "NOISE CURVE" };
    drawSpaced (g, titles[lane], 36.0f, 219.0f, 14.0f, juce::Colours::white, 5.0f);
    g.setColour (kGreenDeep);
    g.fillRect (25, 238, 947, 278);

    // ---- layer mix ----
    g.setColour (kGrayLight);
    g.fillRect (993, 203, 245, 317);
    g.setColour (kGrayDark);
    g.drawRect (993, 203, 245, 317, 2);
    g.setColour (kGreen);
    g.fillRect (993, 203, 245, 32);
    drawSpaced (g, "LAYER MIX", 1008.0f, 219.0f, 14.0f, juce::Colours::white, 5.0f);
    const char* mixNames[4] = { "RATE", "PITCH", "NOISE", "FX" };
    const int mixX[4] = { 1031, 1087, 1144, 1200 };
    for (int i = 0; i < 4; ++i)
        drawSpaced (g, mixNames[i], (float) mixX[i] - 30.0f, 258.0f, 11.0f, kGreenText, 2.0f, 60.0f);

    // ---- paneles RATE y GLOBAL ----
    const juce::Colour panel (0xffcdd1d3);
    g.setColour (panel);
    g.fillRect (22, 535, 1215, 190);
    g.fillRect (22, 733, 1215, 125);
    g.setColour (kGrayDark);
    g.drawRect (22, 535, 1215, 190, 1);
    g.drawRect (22, 733, 1215, 125, 1);
    drawSpaced (g, "RATE", 36.0f, 551.0f, 12.0f, kGreenText, 6.0f);
    drawSpaced (g, "GLOBAL", 36.0f, 749.0f, 12.0f, kGreenText, 6.0f);

    const float lab = 11.0f;
    drawSpaced (g, "MODE", 35.0f, 581.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "START RATE", 272.0f, 581.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "END RATE", 486.0f, 581.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "SHAPE", 35.0f, 658.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "ZAP DEPTH", 380.0f, 658.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "ZAP LENGTH", 820.0f, 658.0f, lab, kGreenText, 4.0f);
    g.setColour (kGrayMid);
    g.fillRect (363, 650, 2, 66);
    g.fillRect (803, 650, 2, 66);

    drawSpaced (g, "LENGTH (BARS)", 35.0f, 780.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "SYNC", 580.0f, 780.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "ON", 620.0f, 814.0f, 12.0f, kGreenText, 4.0f);
    drawSpaced (g, "TRIGGER", 668.0f, 780.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "END GAP", 785.0f, 780.0f, lab, kGreenText, 4.0f);
    drawSpaced (g, "TAIL", 1000.0f, 780.0f, lab, kGreenText, 4.0f);

    // ---- estado ----
    drawSpaced (g, "STATUS:", 416.0f, 877.0f, 12.0f, kGreenText, 4.0f);
    drawSpaced (g, status, 497.0f, 877.0f, 17.0f, kGreenText, 3.0f);
}

//==============================================================================
TuxiroEditor::TuxiroEditor (TuxiroProcessor& p)
    : juce::AudioProcessorEditor (&p), content (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    setResizeLimits (630, 457, 1890, 1372);
    getConstrainer()->setFixedAspectRatio (1260.0 / 915.0);
    setResizable (true, true);
    setSize (882, 641);
}

void TuxiroEditor::resized()
{
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / 1260.0f));
    content.setBounds (0, 0, 1260, 915);
}
