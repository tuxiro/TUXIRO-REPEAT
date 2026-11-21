#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray TuxiroProcessor::presetNames {
    "Supersonic Sweep", "Laser Rise", "Pulse Build", "Noise Wash",
    "Slow Burn", "Gate Chop", "Zap Storm"
};

namespace
{
    // Orden de los valores de cada preset
    const char* const kPresetKeys[20] = {
        "length", "mode", "shape", "startRate", "endRate", "zapDepth", "zapLen",
        "rateCurve", "pitchCurve", "noiseCurve", "pStart", "pEnd", "nStart", "nEnd",
        "mixRate", "mixPitch", "mixNoise", "mixFx", "endGap", "tail"
    };

    struct PresetData { float v[20]; };

    const PresetData kPresets[7] = {
        //  len mode shp sR eR  zapD zapL rC pC nC pS pE   nS   nE   mR   mP   mN   mFx  gap tail
        {{ 3, 0, 0, 2, 6,  72,  0, 0, 0, 0, 0, 36, 0.2f, 1.0f, 0.55f, 0.7f, 0.45f, 0.35f, 2, 1 }},
        {{ 2, 0, 2, 1, 6,  48, 50, 1, 0, 1, 0, 48, 0.1f, 0.8f, 0.6f, 0.5f, 0.4f, 0.4f, 2, 1 }},
        {{ 3, 1, 3, 3, 6,   0,  0, 0, 0, 0, 0, 24, 0.3f, 0.9f, 0.7f, 0.5f, 0.5f, 0.25f, 2, 1 }},
        {{ 4, 2, 1, 0, 4,   0,  0, 1, 0, 0, 0, 12, 0.1f, 1.0f, 0.2f, 0.4f, 0.9f, 0.5f, 0, 1 }},
        {{ 5, 1, 1, 0, 6,   0,  0, 5, 5, 1, 0, 36, 0.0f, 0.9f, 0.5f, 0.6f, 0.5f, 0.4f, 3, 1 }},
        {{ 3, 1, 0, 4, 6,   0,  0, 3, 3, 0, 0, 36, 0.2f, 0.7f, 0.6f, 0.3f, 0.35f, 0.2f, 2, 0 }},
        {{ 2, 0, 0, 2, 6,  60, 80, 6, 4, 4, 12, 48, 0.3f, 1.0f, 0.7f, 0.4f, 0.5f, 0.3f, 1, 1 }},
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout TuxiroProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def)); };
    auto flt = [&] (const char* id, const char* name, float lo, float hi, float def, float step)
    { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, step), def)); };

    const StringArray rates { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    const StringArray curves { "Linear", "Exp", "Log", "Steps", "S", "Late", "Pump", "Wobble" };

    choice ("length",    "Length (bars)", { "1", "2", "4", "8", "16", "32", "64" }, 3);
    choice ("mode",      "Mode",          { "LASER", "PULSE", "SMOOTH" }, 0);
    choice ("shape",     "Shape",         { "Square", "Sine", "Saw", "Pulse" }, 0);
    choice ("startRate", "Start Rate",    rates, 2);
    choice ("endRate",   "End Rate",      rates, 6);
    flt    ("zapDepth",  "Zap Depth",     0.0f, 72.0f, 72.0f, 1.0f);
    flt    ("zapLen",    "Zap Length",    0.0f, 100.0f, 0.0f, 1.0f);
    choice ("rateCurve",  "Rate Curve",  curves, 0);
    choice ("pitchCurve", "Pitch Curve", curves, 0);
    choice ("noiseCurve", "Noise Curve", curves, 0);
    flt    ("pStart", "Pitch Start", 0.0f, 60.0f, 0.0f, 0.1f);
    flt    ("pEnd",   "Pitch End",   0.0f, 60.0f, 36.0f, 0.1f);
    flt    ("nStart", "Noise Start", 0.0f, 1.0f, 0.2f, 0.01f);
    flt    ("nEnd",   "Noise End",   0.0f, 1.0f, 1.0f, 0.01f);
    flt    ("mixRate",  "Mix Rate",  0.0f, 1.0f, 0.55f, 0.01f);
    flt    ("mixPitch", "Mix Pitch", 0.0f, 1.0f, 0.70f, 0.01f);
    flt    ("mixNoise", "Mix Noise", 0.0f, 1.0f, 0.45f, 0.01f);
    flt    ("mixFx",    "Mix FX",    0.0f, 1.0f, 0.35f, 0.01f);
    choice ("endGap",  "End Gap", { "OFF", "1/16", "1/8", "1/4", "1/2", "1/1" }, 2);
    choice ("tail",    "Tail",    { "CUT", "RING OUT" }, 1);
    choice ("trigger", "Trigger", { "GATE", "ONE-SHOT", "HOST" }, 1);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Sync", true));
    return l;
}

TuxiroProcessor::TuxiroProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    apvts.state.setProperty ("preset", 0, nullptr);
}

void TuxiroProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    previewing = false;
    activeNote = -1;
}

bool TuxiroProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

trx::RiserParams TuxiroProcessor::readParams() const
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto idx = [&] (const char* id, int maxIdx) { return juce::jlimit (0, maxIdx, (int) std::lround (f (id))); };

    trx::RiserParams p;
    p.lengthBars = trx::kLengthBars[idx ("length", 6)];
    p.mode  = idx ("mode", 2);
    p.shape = idx ("shape", 3);
    p.startRate = (float) idx ("startRate", 6);
    p.endRate   = (float) idx ("endRate", 6);
    p.zapDepth  = f ("zapDepth");
    p.zapLength = f ("zapLen");
    p.curve[0] = idx ("rateCurve", 7);
    p.curve[1] = idx ("pitchCurve", 7);
    p.curve[2] = idx ("noiseCurve", 7);
    p.pStart = f ("pStart");  p.pEnd = f ("pEnd");
    p.nStart = f ("nStart");  p.nEnd = f ("nEnd");
    p.mixRate = f ("mixRate");   p.mixPitch = f ("mixPitch");
    p.mixNoise = f ("mixNoise"); p.mixFx = f ("mixFx");
    p.gapBeats = trx::kGapBeats[idx ("endGap", 5)];
    p.ringOut = idx ("tail", 1) == 1;
    return p;
}

void TuxiroProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    const auto p = readParams();
    engine.setParams (p);

    double hostBpm = 0.0, ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) hostBpm = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
        }

    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    const int  trig = juce::jlimit (0, 2, (int) std::lround (apvts.getRawParameterValue ("trigger")->load()));

    double bpm = 128.0;
    if (hostBpm > 1.0 && (sync || trig == 2)) bpm = hostBpm;
    uiBpm = (float) bpm;

    // Boton PLAY/STOP de la interfaz (preview)
    const int cmd = uiCommand.exchange (0);
    if (cmd == 1)      { engine.start (0.0f); previewing = true; }
    else if (cmd == 2) { engine.stop(); previewing = false; }

    // Modo HOST: el riser sigue la posicion del transporte, en bucle cada "Length" compases
    const double total = p.lengthBars * 4.0;
    if (trig == 2 && ! previewing)
    {
        if (playing && havePpq && ppq >= 0.0)
        {
            const double b = std::fmod (ppq, total);
            if (! engine.isRunning() || std::abs (engine.getPosBeats() - b) > 0.1)
                engine.start (0.0f, b);
        }
        else if (engine.isRunning())
            engine.stop();
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0);

    int cursor = 0;
    auto renderTo = [&] (int end)
    {
        end = juce::jlimit (cursor, n, end);
        if (end > cursor)
        {
            engine.render (L + cursor, R + cursor, end - cursor, bpm);
            cursor = end;
        }
    };

    if (trig != 2)
    {
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                renderTo (metadata.samplePosition);
                engine.start ((float) (msg.getNoteNumber() - 60));
                activeNote = msg.getNoteNumber();
                previewing = false;
            }
            else if (msg.isNoteOff() && trig == 0 && msg.getNoteNumber() == activeNote)
            {
                renderTo (metadata.samplePosition);
                engine.stop();
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                renderTo (metadata.samplePosition);
                engine.stop();
            }
        }
    }
    renderTo (n);

    if (! engine.isRunning()) previewing = false;
    uiFraction = engine.getFraction();
    uiState = engine.isRunning() ? (engine.isInGap() ? 2 : 1) : 0;
}

// ------------------------------------------------------------------ UI helpers

void TuxiroProcessor::setParamPlain (const juce::String& id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
        prm->endChangeGesture();
    }
}

void TuxiroProcessor::snapshotUndo()
{
    undoState = apvts.copyState();
}

void TuxiroProcessor::undo()
{
    if (! undoState.isValid()) return;
    auto current = apvts.copyState();
    apvts.replaceState (undoState);
    undoState = current;
}

void TuxiroProcessor::applyPreset (int index)
{
    if (index < 0 || index >= presetNames.size()) return;
    snapshotUndo();
    for (int i = 0; i < 20; ++i)
        setParamPlain (kPresetKeys[i], kPresets[index].v[i]);
    apvts.state.setProperty ("preset", index, nullptr);
}

void TuxiroProcessor::randomize()
{
    snapshotUndo();
    auto& r = juce::Random::getSystemRandom();
    setParamPlain ("mode", (float) r.nextInt (3));
    setParamPlain ("shape", (float) r.nextInt (4));
    setParamPlain ("startRate", (float) r.nextInt (4));
    setParamPlain ("endRate", (float) (4 + r.nextInt (3)));
    setParamPlain ("zapDepth", (float) (12 * r.nextInt (7)));
    setParamPlain ("zapLen", r.nextFloat() * 100.0f);
    setParamPlain ("rateCurve", (float) r.nextInt (8));
    setParamPlain ("pitchCurve", (float) r.nextInt (8));
    setParamPlain ("noiseCurve", (float) r.nextInt (8));
    setParamPlain ("pStart", r.nextFloat() * 12.0f);
    setParamPlain ("pEnd", 12.0f + r.nextFloat() * 48.0f);
    setParamPlain ("nStart", r.nextFloat() * 0.4f);
    setParamPlain ("nEnd", 0.5f + r.nextFloat() * 0.5f);
    setParamPlain ("mixRate", 0.3f + r.nextFloat() * 0.6f);
    setParamPlain ("mixPitch", 0.3f + r.nextFloat() * 0.6f);
    setParamPlain ("mixNoise", 0.3f + r.nextFloat() * 0.6f);
    setParamPlain ("mixFx", 0.2f + r.nextFloat() * 0.5f);
    apvts.state.setProperty ("preset", -1, nullptr);
}

bool TuxiroProcessor::renderToWav (const juce::File& dest)
{
    juce::ScopedNoDenormals noDenormals;
    const auto p = readParams();
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    const double bpm = juce::jmax (40.0, (double) uiBpm.load());

    trx::RiserEngine e;
    e.prepare (sr);
    e.setParams (p);
    e.start (0.0f);

    const double secs = p.lengthBars * 4.0 * 60.0 / bpm + (p.ringOut ? 3.0 : 0.0);
    const int total = (int) (secs * sr);
    juce::AudioBuffer<float> buf (2, total);
    buf.clear();

    for (int pos = 0; pos < total;)
    {
        const int len = juce::jmin (1024, total - pos);
        e.render (buf.getWritePointer (0) + pos, buf.getWritePointer (1) + pos, len, bpm);
        pos += len;
    }

    const float peak = buf.getMagnitude (0, total);
    if (peak > 0.0001f) buf.applyGain (0.89f / peak);   // normaliza a -1 dBFS

    dest.getParentDirectory().createDirectory();
    dest.deleteFile();
    auto* stream = new juce::FileOutputStream (dest);
    if (! stream->openedOk()) { delete stream; return false; }

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream, sr, 2, 24, {}, 0));
    if (writer == nullptr) return false;
    return writer->writeFromAudioSampleBuffer (buf, 0, total);
}

void TuxiroProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TuxiroProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TuxiroProcessor::createEditor()
{
    return new TuxiroEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TuxiroProcessor();
}
