#pragma once

#include "Common.h"

namespace sk
{
enum class StretchMode : int
{
    Hybrid = 0, // attack kept untouched, only the tail is time-stretched (best for drums)
    Smooth,     // whole sample through Rubber Band R3
    Tape        // varispeed: length and pitch change together, like slowing a turntable
};

struct RenderSettings
{
    double sourceRate     = 44100.0;
    double targetRate     = 44100.0;
    float  pitchSemitones = 0.0f;   // classic sampler pitch (resampling). Ignored in Tape mode.
    float  lengthPercent  = 100.0f; // final length relative to the ORIGINAL sample length
    StretchMode mode      = StretchMode::Hybrid;
};

/** Offline renderer: everything happens on a background thread before the
    sample is played, so we can use the highest-quality settings. */
class DrumStretcher
{
public:
    static juce::AudioBuffer<float> render (const juce::AudioBuffer<float>& source, const RenderSettings& settings);

    /** speed = input samples consumed per output sample (2.0 = twice as fast / half as long). */
    static juce::AudioBuffer<float> resample (const juce::AudioBuffer<float>& in, double speed);

    /** Rubber Band R3 offline stretch. timeRatio 2.0 = twice as long. */
    static juce::AudioBuffer<float> rubberBandStretch (const juce::AudioBuffer<float>& in, double sampleRate,
                                                       double timeRatio, double pitchScale = 1.0);

    /** Sample index where the transient (attack) ends and the tail begins. */
    static int findAttackEnd (const juce::AudioBuffer<float>& in, double sampleRate);

private:
    static void fadeOutEnd (juce::AudioBuffer<float>& b, int fadeSamples);
};
} // namespace sk
