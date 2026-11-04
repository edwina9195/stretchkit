#pragma once

#include "Common.h"

#include <optional>

namespace sk
{
/** Indices into SampleFeatures::v. All values are roughly 0..1 or log-scaled. */
enum FeatureIndex
{
    fLogCentroid = 0, // spectral centroid (log Hz, normalised)
    fLogRolloff,      // 85% roll-off (log Hz, normalised)
    fFlatness,        // 0 = tonal, 1 = noise
    fLow,             // energy share < 120 Hz
    fLowMid,          // 120 Hz - 1 kHz
    fHighMid,         // 1 kHz - 5 kHz
    fHigh,            // > 5 kHz
    fLogDecay,        // time to fall 30 dB below the peak
    fLogAttack,       // time from 10% to peak
    fZcr              // zero-crossing rate
};

struct SampleFeatures
{
    std::array<float, kNumFeatures> v {};
    float durationSec = 0.0f;
    float centroidHz  = 0.0f;
    float decaySec    = 0.0f;
    int   bursts      = 0;   // separate peaks in the first ~40 ms (claps)
};

class FeatureExtractor
{
public:
    /** Analyses the first few seconds of a one-shot. */
    static SampleFeatures analyse (const juce::AudioBuffer<float>& buffer, double sampleRate, float durationSec);

    /** Uses file / folder names first ("kick", "BD_01", "Snares/..."), then falls back to the sound itself. */
    static Category classify (const juce::File& file, const SampleFeatures& f);

    static std::optional<Category> classifyByName (const juce::File& file);
    static Category classifyBySound (const SampleFeatures& f);
};
} // namespace sk
