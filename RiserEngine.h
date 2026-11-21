#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace trx
{
inline constexpr int   kLengthBars[7] = { 1, 2, 4, 8, 16, 32, 64 };
inline constexpr float kGapBeats[6]   = { 0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };   // OFF,1/16,1/8,1/4,1/2,1/1

// Formas de curva (0..1 -> 0..1). Todas empiezan en 0 y acaban en 1.
inline float curveShape (int type, float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    const float twoPi = juce::MathConstants<float>::twoPi;
    switch (type)
    {
        case 0:  return t;                                            // lineal
        case 1:  return std::pow (t, 2.5f);                           // exponencial
        case 2:  return 1.0f - std::pow (1.0f - t, 2.5f);             // logaritmica
        case 3:  return std::min (1.0f, std::floor (t * 8.0f) / 7.0f); // escalones
        case 4:  return t * t * (3.0f - 2.0f * t);                    // S
        case 5:  return std::pow (t, 6.0f);                           // tardia
        case 6:  return juce::jlimit (0.0f, 1.0f, t + 0.08f * std::sin (twoPi * 4.0f * t));
        default: return juce::jlimit (0.0f, 1.0f, t + 0.12f * std::sin (twoPi * 3.0f * t)
                                                      + 0.05f * std::sin (twoPi * 7.0f * t));
    }
}

struct RiserParams
{
    int   lengthBars = 8;
    int   mode  = 0;               // 0 LASER, 1 PULSE, 2 SMOOTH
    int   shape = 0;               // 0 square, 1 sine, 2 saw, 3 pulse
    float startRate = 2.0f, endRate = 6.0f;   // indice 0..6 = 1/1 .. 1/64
    float zapDepth = 72.0f, zapLength = 0.0f; // semitonos, %
    int   curve[3] = { 0, 0, 0 };  // rate, pitch, noise
    float pStart = 0.0f, pEnd = 36.0f;        // semitonos
    float nStart = 0.2f, nEnd = 1.0f;         // 0..1
    float mixRate = 0.55f, mixPitch = 0.7f, mixNoise = 0.45f, mixFx = 0.35f;
    float gapBeats = 0.5f;
    bool  ringOut = true;
};

class RiserEngine
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        gateCoef = 1.0f - std::exp (-1.0f / (0.0008f * (float) sr));
        reverb.setSampleRate (sr);
        reverb.reset();
        running = false;
        idle = 0;
    }

    void setParams (const RiserParams& p)
    {
        params = p;
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.82f;
        rp.damping  = 0.35f;
        rp.wetLevel = p.mixFx * 0.33f;
        rp.dryLevel = 0.5f;
        rp.width    = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }

    void start (float transposeSemis, double startBeat = 0.0)
    {
        rootOffset = transposeSemis;
        pos = startBeat;
        running = true;
        stopping = false;
        stopGain = 1.0f;
        hitPhase = 0.0;
        oscPhase = 0.0;
        sweepPhase = 0.0;
        hpLp = 0.0f;
        gateSm = 0.0f;
        gapSm = 1.0f;
        cutDone = false;
        idle = 0;
    }

    void stop() { if (running) stopping = true; }

    bool   isRunning() const    { return running; }
    double getPosBeats() const  { return pos; }
    bool   isInGap() const
    {
        const double total = params.lengthBars * 4.0;
        const double gap = std::min<double> (params.gapBeats, total * 0.5);
        return running && pos >= total - gap;
    }
    float  getFraction() const
    {
        return running ? (float) juce::jlimit (0.0, 1.0, pos / (params.lengthBars * 4.0)) : 0.0f;
    }

    void render (float* L, float* R, int n, double bpm)
    {
        const double dBeat = bpm / 60.0 / sampleRate;
        const double total = params.lengthBars * 4.0;
        const double gap   = std::min<double> (params.gapBeats, total * 0.5);
        const double rise  = total - gap;
        const float  twoPi = juce::MathConstants<float>::twoPi;
        const float  sr    = (float) sampleRate;

        for (int i = 0; i < n; ++i)
        {
            float out = 0.0f;

            if (running)
            {
                idle = 0;
                const bool  inGap = pos >= rise;
                const float t = (float) juce::jlimit (0.0, 1.0, pos / rise);

                if (inGap && ! params.ringOut && ! cutDone)
                {
                    reverb.reset();      // CUT: se corta la cola en el silencio previo al drop
                    cutDone = true;
                }

                auto lane = [&] (int k, float a, float b) { return a + (b - a) * curveShape (params.curve[k], t); };
                const float rateIdx = lane (0, params.startRate, params.endRate);
                const float semis   = lane (1, params.pStart, params.pEnd);
                const float nz      = lane (2, params.nStart, params.nEnd);

                // ---- repeticiones (aceleracion) ----
                hitPhase += std::exp2 (rateIdx - 2.0f) * dBeat;     // 1/4 = 1 golpe por beat
                if (hitPhase >= 1.0)
                {
                    hitPhase -= std::floor (hitPhase);
                    if (params.mode != 2) oscPhase = 0.0;
                }

                const double duty = params.mode == 0 ? 0.55 : 0.45;
                const float gate = (params.mode == 2 || hitPhase < duty) ? 1.0f : 0.0f;
                gateSm += (gate - gateSm) * gateCoef;

                // ---- zap (laser) ----
                float zap = 0.0f;
                if (params.mode == 0)
                {
                    const float zl = 0.12f + 0.88f * params.zapLength / 100.0f;
                    const float x = std::max (0.0f, 1.0f - (float) hitPhase / zl);
                    zap = params.zapDepth * x * x;
                }

                const float note = juce::jlimit (12.0f, 118.0f, 48.0f + rootOffset + semis + zap);
                const double inc = 440.0 * std::exp2 ((note - 69.0f) / 12.0f) / sampleRate;
                oscPhase += inc;
                if (oscPhase >= 1.0) oscPhase -= 1.0;
                const float hits = osc (params.shape, oscPhase, inc);

                // ---- capa de barrido continuo (sinusoide) ----
                const float sNote = juce::jlimit (12.0f, 118.0f, 60.0f + rootOffset + semis);
                sweepPhase += 440.0 * std::exp2 ((sNote - 69.0f) / 12.0f) / sampleRate;
                if (sweepPhase >= 1.0) sweepPhase -= 1.0;
                const float sweep = std::sin (twoPi * (float) sweepPhase);

                // ---- ruido con paso alto que sube ----
                noiseSeed ^= noiseSeed << 13;
                noiseSeed ^= noiseSeed >> 17;
                noiseSeed ^= noiseSeed << 5;
                const float w = (float) (int32_t) noiseSeed / 2147483648.0f;
                const float fc = 500.0f * std::exp2 (t * 4.2f);
                const float g  = 1.0f - std::exp (-twoPi * fc / sr);
                hpLp += g * (w - hpLp);
                const float hp = w - hpLp;
                const float noise = hp * nz * (params.mode == 2 ? 1.0f : (0.35f + 0.65f * gateSm));

                gapSm += ((inGap ? 0.0f : 1.0f) - gapSm) * gateCoef;

                const float mix = hits  * gateSm * params.mixRate * 0.5f
                                + sweep * params.mixPitch * 0.3f * (0.4f + 0.6f * t)
                                + noise * params.mixNoise * 0.45f;
                const float envT = 0.15f + 0.85f * t;
                out = std::tanh (mix * envT * gapSm * stopGain * 1.4f) * 0.9f;

                pos += dBeat;
                if (stopping)
                {
                    stopGain -= 1.0f / (0.03f * sr);
                    if (stopGain <= 0.0f) { stopGain = 0.0f; running = false; }
                }
                if (pos >= total) running = false;
            }
            else
            {
                ++idle;
            }

            if (! running && (double) idle > 6.0 * sampleRate)
            {
                L[i] = R[i] = 0.0f;       // la cola del reverb ya se apago
                continue;
            }

            float l = out, r = out;
            reverb.processStereo (&l, &r, 1);
            L[i] = l;
            R[i] = r;
        }
    }

private:
    static float polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt;           return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    static float osc (int shape, double ph, double inc)
    {
        switch (shape)
        {
            case 0:
            {
                float v = ph < 0.5 ? 1.0f : -1.0f;
                v += polyBlep (ph, inc);
                v -= polyBlep (std::fmod (ph + 0.5, 1.0), inc);
                return v * 0.8f;
            }
            case 1:  return std::sin (juce::MathConstants<float>::twoPi * (float) ph);
            case 2:  return ((float) (2.0 * ph - 1.0) - polyBlep (ph, inc)) * 0.8f;
            default:
            {
                float v = ph < 0.25 ? 1.0f : -1.0f;
                v += polyBlep (ph, inc);
                v -= polyBlep (std::fmod (ph + 0.75, 1.0), inc);
                return v * 0.8f;
            }
        }
    }

    RiserParams params;
    juce::Reverb reverb;
    double sampleRate = 44100.0;
    float  gateCoef = 0.05f;

    bool   running = false, stopping = false, cutDone = false;
    double pos = 0.0, hitPhase = 0.0, oscPhase = 0.0, sweepPhase = 0.0;
    float  rootOffset = 0.0f, stopGain = 1.0f, gateSm = 0.0f, gapSm = 1.0f, hpLp = 0.0f;
    uint32_t noiseSeed = 0x12345678u;
    juce::int64 idle = 0;
};
} // namespace trx
