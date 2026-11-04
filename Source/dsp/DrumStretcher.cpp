#include "DrumStretcher.h"

#include <rubberband/RubberBandStretcher.h>

#include <cmath>
#include <vector>

namespace sk
{
namespace
{
juce::AudioBuffer<float> copyRange (const juce::AudioBuffer<float>& src, int start, int length)
{
    start  = juce::jlimit (0, src.getNumSamples(), start);
    length = juce::jlimit (0, src.getNumSamples() - start, length);
    juce::AudioBuffer<float> out (src.getNumChannels(), length);
    for (int c = 0; c < src.getNumChannels(); ++c)
        out.copyFrom (c, 0, src, c, start, length);
    return out;
}

/** Returns a copy resized to exactly `length` samples (zero-padded or truncated). */
juce::AudioBuffer<float> fitLength (const juce::AudioBuffer<float>& src, int length)
{
    juce::AudioBuffer<float> out (src.getNumChannels(), juce::jmax (0, length));
    out.clear();
    const int n = juce::jmin (length, src.getNumSamples());
    for (int c = 0; c < src.getNumChannels(); ++c)
        if (n > 0)
            out.copyFrom (c, 0, src, c, 0, n);
    return out;
}
} // namespace

//==============================================================================
juce::AudioBuffer<float> DrumStretcher::render (const juce::AudioBuffer<float>& source, const RenderSettings& s)
{
    const int srcLen = source.getNumSamples();
    if (srcLen == 0 || source.getNumChannels() == 0)
        return {};

    const double rateRatio   = s.sourceRate / s.targetRate;                 // input samples per output sample at 1x
    const double lengthScale = juce::jlimit (0.05, 10.0, (double) s.lengthPercent / 100.0);
    const int    targetLen   = juce::jmax (1, (int) std::lround ((double) srcLen / rateRatio * lengthScale));
    const int    fade        = (int) (0.002 * s.targetRate);

    if (s.mode == StretchMode::Tape)
    {
        auto out = fitLength (resample (source, (double) srcLen / (double) targetLen), targetLen);
        fadeOutEnd (out, fade);
        return out;
    }

    // 1) classic sampler pitch + conversion to the host sample rate
    const double pitchFactor = std::pow (2.0, (double) s.pitchSemitones / 12.0);
    auto pitched = resample (source, rateRatio * pitchFactor);
    const int n = pitched.getNumSamples();
    if (n == 0)
        return {};

    const double timeRatio = (double) targetLen / (double) n;

    if (std::abs (timeRatio - 1.0) < 0.002)
    {
        auto out = fitLength (pitched, targetLen);
        fadeOutEnd (out, fade);
        return out;
    }

    // 2a) Smooth: whole sample through Rubber Band
    if (s.mode == StretchMode::Smooth)
    {
        auto out = fitLength (rubberBandStretch (pitched, s.targetRate, timeRatio), targetLen);
        fadeOutEnd (out, fade);
        return out;
    }

    // 2b) Hybrid: keep the transient sample-for-sample, stretch only the tail
    const int attackEnd = findAttackEnd (pitched, s.targetRate);
    const int xfade     = juce::jmin ((int) (0.004 * s.targetRate), attackEnd);
    const int tailStart = attackEnd - xfade;
    const int tailLen   = n - tailStart;

    // Sample too short to have a tail worth stretching, or target shorter than the attack:
    // play the original and cut it with a short fade.
    if (tailLen <= xfade * 2 || targetLen <= attackEnd + xfade)
    {
        auto out = fitLength (pitched, targetLen);
        fadeOutEnd (out, fade);
        return out;
    }

    const double tailRatio = (double) (targetLen - tailStart) / (double) tailLen;
    auto tail = rubberBandStretch (copyRange (pitched, tailStart, tailLen), s.targetRate, tailRatio);

    juce::AudioBuffer<float> out (pitched.getNumChannels(), targetLen);
    out.clear();

    for (int c = 0; c < out.getNumChannels(); ++c)
    {
        const float* orig = pitched.getReadPointer (c);
        const float* str  = tail.getReadPointer (juce::jmin (c, tail.getNumChannels() - 1));
        float* dst        = out.getWritePointer (c);
        const int strLen  = tail.getNumSamples();

        // untouched attack
        std::copy (orig, orig + tailStart, dst);

        // equal-power crossfade original -> stretched
        for (int i = 0; i < xfade; ++i)
        {
            const float w  = (float) (i + 1) / (float) (xfade + 1);
            const float gA = std::cos (w * juce::MathConstants<float>::halfPi);
            const float gB = std::sin (w * juce::MathConstants<float>::halfPi);
            const float b  = i < strLen ? str[i] : 0.0f;
            dst[tailStart + i] = orig[tailStart + i] * gA + b * gB;
        }

        // stretched tail
        for (int i = xfade; i < targetLen - tailStart; ++i)
            dst[tailStart + i] = i < strLen ? str[i] : 0.0f;
    }

    fadeOutEnd (out, fade);
    return out;
}

//==============================================================================
juce::AudioBuffer<float> DrumStretcher::resample (const juce::AudioBuffer<float>& in, double speed)
{
    const int n  = in.getNumSamples();
    const int ch = in.getNumChannels();
    if (n == 0 || ch == 0 || speed <= 0.0)
        return {};

    if (std::abs (speed - 1.0) < 1.0e-9)
        return copyRange (in, 0, n);

    const int outLen  = juce::jmax (1, (int) std::floor ((double) n / speed));
    const float lat   = juce::WindowedSincInterpolator::getBaseLatency();     // in input samples
    const int skip    = (int) std::lround ((double) lat / speed);
    const int produce = outLen + skip;
    const int padIn   = (int) std::ceil ((double) produce * speed) + 64;

    juce::AudioBuffer<float> out (ch, outLen);
    std::vector<float> padded ((size_t) padIn, 0.0f);
    std::vector<float> tmp ((size_t) produce, 0.0f);

    for (int c = 0; c < ch; ++c)
    {
        std::fill (padded.begin(), padded.end(), 0.0f);
        std::copy (in.getReadPointer (c), in.getReadPointer (c) + n, padded.begin());

        juce::WindowedSincInterpolator interp;
        interp.process (speed, padded.data(), tmp.data(), produce);
        out.copyFrom (c, 0, tmp.data() + skip, outLen);
    }

    return out;
}

//==============================================================================
juce::AudioBuffer<float> DrumStretcher::rubberBandStretch (const juce::AudioBuffer<float>& in, double sampleRate,
                                                           double timeRatio, double pitchScale)
{
    using RB = RubberBand::RubberBandStretcher;

    const int n  = in.getNumSamples();
    const int ch = in.getNumChannels();
    if (n < 64 || ch == 0)
        return copyRange (in, 0, n);

    RB rb ((size_t) std::lround (sampleRate), (size_t) ch,
           RB::OptionProcessOffline | RB::OptionEngineFiner | RB::OptionChannelsTogether,
           timeRatio, pitchScale);

    constexpr int block = 1024;
    rb.setExpectedInputDuration ((size_t) n);
    rb.setMaxProcessSize ((size_t) block);

    std::vector<const float*> inPtrs ((size_t) ch);

    for (int pos = 0; pos < n; pos += block)
    {
        const int count = juce::jmin (block, n - pos);
        for (int c = 0; c < ch; ++c)
            inPtrs[(size_t) c] = in.getReadPointer (c, pos);
        rb.study (inPtrs.data(), (size_t) count, pos + count >= n);
    }

    juce::AudioBuffer<float> out (ch, (int) std::ceil (n * timeRatio) + block * 4);
    out.clear();
    int written = 0;
    std::vector<float*> outPtrs ((size_t) ch);

    auto drain = [&]
    {
        for (int avail = (int) rb.available(); avail > 0; avail = (int) rb.available())
        {
            if (written + avail > out.getNumSamples())
                out.setSize (ch, written + avail + block, true, true);

            for (int c = 0; c < ch; ++c)
                outPtrs[(size_t) c] = out.getWritePointer (c, written);

            written += (int) rb.retrieve (outPtrs.data(), (size_t) avail);
        }
    };

    for (int pos = 0; pos < n; pos += block)
    {
        const int count = juce::jmin (block, n - pos);
        for (int c = 0; c < ch; ++c)
            inPtrs[(size_t) c] = in.getReadPointer (c, pos);
        rb.process (inPtrs.data(), (size_t) count, pos + count >= n);
        drain();
    }
    drain();

    out.setSize (ch, written, true);
    return out;
}

//==============================================================================
int DrumStretcher::findAttackEnd (const juce::AudioBuffer<float>& in, double sr)
{
    const int n = in.getNumSamples();
    if (n == 0)
        return 0;

    // mono rectified envelope with a ~1 ms release
    std::vector<float> env ((size_t) n);
    const float rel = std::exp (-1.0f / (float) (0.001 * sr));
    float e = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float a = 0.0f;
        for (int c = 0; c < in.getNumChannels(); ++c)
            a = juce::jmax (a, std::abs (in.getSample (c, i)));
        e = a > e ? a : a + (e - a) * rel;
        env[(size_t) i] = e;
    }

    const int searchEnd = juce::jmin (n, (int) (0.08 * sr));
    int peak = 0;
    for (int i = 1; i < searchEnd; ++i)
        if (env[(size_t) i] > env[(size_t) peak])
            peak = i;

    const float peakVal = env[(size_t) peak];
    const int minHold = peak + (int) (0.010 * sr);
    const int maxHold = peak + (int) (0.040 * sr);

    int end = minHold;
    while (end < maxHold && end < n && env[(size_t) end] > peakVal * 0.5f)
        ++end;

    return juce::jlimit (0, n / 2, end);
}

void DrumStretcher::fadeOutEnd (juce::AudioBuffer<float>& b, int fadeSamples)
{
    const int n = b.getNumSamples();
    fadeSamples = juce::jmin (fadeSamples, n);
    if (fadeSamples > 0)
        b.applyGainRamp (n - fadeSamples, fadeSamples, 1.0f, 0.0f);
}
} // namespace sk
