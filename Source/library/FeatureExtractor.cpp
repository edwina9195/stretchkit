#include "FeatureExtractor.h"

#include <cmath>
#include <vector>

namespace sk
{
namespace
{
float norm01 (float x, float lo, float hi) { return juce::jlimit (0.0f, 1.0f, (x - lo) / (hi - lo)); }

/** "OpenHat_BD02-tight" -> { "open", "hat", "bd", "02", "tight" } */
juce::StringArray tokenise (const juce::String& text)
{
    juce::StringArray tokens;
    juce::String cur;
    juce::juce_wchar prev = 0;

    auto flush = [&] { if (cur.isNotEmpty()) tokens.add (cur.toLowerCase()); cur.clear(); };

    for (auto p = text.getCharPointer(); ! p.isEmpty(); ++p)
    {
        const auto ch = *p;
        if (! juce::CharacterFunctions::isLetterOrDigit (ch))
        {
            flush();
        }
        else
        {
            const bool boundary = cur.isNotEmpty()
                && ((juce::CharacterFunctions::isDigit (ch) != juce::CharacterFunctions::isDigit (prev))
                    || (juce::CharacterFunctions::isUpperCase (ch) && juce::CharacterFunctions::isLowerCase (prev)));
            if (boundary)
                flush();
            cur += juce::String::charToString (ch);
        }
        prev = ch;
    }
    flush();
    return tokens;
}

struct Rule
{
    Category cat;
    const char* exact;      // space separated tokens that must match a whole token
    const char* contains;   // space separated substrings searched in the joined lower-case name
};

const Rule rules[] = {
    { Category::Clap,   "clap clp cp claps snap snaps",                        "clap" },
    { Category::HiHat,  "hat hats hh hihat hihats chh ohh hho hhc",            "hihat hi-hat hi_hat openhat closedhat" },
    { Category::Cymbal, "cym cymbal crash ride splash china cymbals",          "cymbal crash" },
    { Category::Snare,  "snare snr sd snares",                                 "snare" },
    { Category::Kick,   "kick kik kck bd bassdrum kicks bdrum",                "kick bassdrum" },
    { Category::Tom,    "tom toms floortom",                                   "floortom" },
    { Category::Perc,   "perc percs percussion shaker shk conga bongo cowbell tamb tambourine rim rimshot "
                        "clave block triangle guiro timbale fx",               "perc shaker conga bongo cowbell tambourine rimshot" },
};

std::optional<Category> matchName (const juce::String& name)
{
    const auto tokens = tokenise (name);
    const auto joined = name.toLowerCase();

    for (const auto& r : rules)
    {
        for (const auto& t : juce::StringArray::fromTokens (r.exact, " ", ""))
            if (tokens.contains (t))
                return r.cat;

        for (const auto& t : juce::StringArray::fromTokens (r.contains, " ", ""))
            if (joined.contains (t))
                return r.cat;
    }
    return std::nullopt;
}
} // namespace

//==============================================================================
SampleFeatures FeatureExtractor::analyse (const juce::AudioBuffer<float>& buffer, double sr, float durationSec)
{
    SampleFeatures f;
    f.durationSec = durationSec;

    const int n = juce::jmin (buffer.getNumSamples(), (int) (6.0 * sr));
    const int ch = buffer.getNumChannels();
    if (n < 64 || ch == 0 || sr <= 0.0)
        return f;

    // mono mix
    std::vector<float> m ((size_t) n, 0.0f);
    for (int c = 0; c < ch; ++c)
    {
        const float* src = buffer.getReadPointer (c);
        for (int i = 0; i < n; ++i)
            m[(size_t) i] += src[i] / (float) ch;
    }

    float maxAbs = 0.0f;
    for (auto s : m) maxAbs = juce::jmax (maxAbs, std::abs (s));
    if (maxAbs < 1.0e-6f)
        return f;

    int onset = 0;
    while (onset < n - 1 && std::abs (m[(size_t) onset]) < 0.05f * maxAbs)
        ++onset;

    // ---- envelope (5 ms RMS) -> attack / decay
    const int w = juce::jmax (1, (int) (0.005 * sr));
    std::vector<float> env;
    for (int i = 0; i < n; i += w)
    {
        double acc = 0.0;
        const int e = juce::jmin (n, i + w);
        for (int j = i; j < e; ++j) acc += (double) m[(size_t) j] * m[(size_t) j];
        env.push_back ((float) std::sqrt (acc / (double) (e - i)));
    }

    int peakW = 0;
    for (int i = 1; i < (int) env.size(); ++i)
        if (env[(size_t) i] > env[(size_t) peakW]) peakW = i;
    const float peak = env[(size_t) peakW];

    int startW = 0;
    while (startW < peakW && env[(size_t) startW] < 0.1f * peak) ++startW;
    const float attackMs = (float) (peakW - startW) * 5.0f + 0.5f;

    int decW = peakW;
    while (decW < (int) env.size() && env[(size_t) decW] > peak * 0.0316f) ++decW;
    f.decaySec = juce::jmax (0.005f, (float) (decW - peakW) * 0.005f);

    // ---- bursts in the first 45 ms, for claps.
    // Envelope = max |x| over a sliding 5 ms window, evaluated every 1 ms (removes ripple above ~200 Hz).
    {
        const int hop1 = juce::jmax (1, (int) (0.001 * sr));
        const int win  = juce::jmax (1, (int) (0.005 * sr));
        std::vector<float> fine;
        for (int i = onset; i < juce::jmin (n, onset + (int) (0.045 * sr)); i += hop1)
        {
            float a = 0.0f;
            for (int j = i; j < juce::jmin (n, i + win); ++j) a = juce::jmax (a, std::abs (m[(size_t) j]));
            fine.push_back (a);
        }

        float lastPeak = 0.0f, runMin = 0.0f;
        bool armed = true;
        for (size_t i = 1; i + 1 < fine.size(); ++i)
        {
            runMin = juce::jmin (runMin, fine[i]);
            if (! armed && runMin < 0.5f * lastPeak)
                armed = true;

            const bool isPeak = fine[i] >= fine[i - 1] && fine[i] > fine[i + 1];
            if (isPeak && armed && fine[i] > 0.2f * maxAbs)
            {
                ++f.bursts;
                lastPeak = fine[i];
                runMin   = fine[i];
                armed    = false;
            }
            else if (fine[i] > lastPeak && ! armed)
            {
                lastPeak = fine[i];
                runMin   = fine[i];
            }
        }
    }

    // ---- spectrum: energy-weighted average over the first 350 ms after the onset
    constexpr int order = 11, fftSize = 1 << order, hop = 512;
    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> frame ((size_t) fftSize * 2);
    std::vector<double> power ((size_t) fftSize / 2 + 1, 0.0);

    const int specEnd = juce::jmin (n, onset + (int) (0.35 * sr));
    for (int start = onset; start == onset || start + fftSize <= specEnd; start += hop)
    {
        std::fill (frame.begin(), frame.end(), 0.0f);
        for (int i = 0; i < fftSize && start + i < n; ++i)
            frame[(size_t) i] = m[(size_t) (start + i)];
        window.multiplyWithWindowingTable (frame.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (frame.data());
        for (size_t b = 0; b < power.size(); ++b)
            power[b] += (double) frame[b] * frame[b];
        if (start + fftSize >= specEnd) break;
    }

    const double binHz = sr / fftSize;
    double total = 0.0, weighted = 0.0, bands[4] = { 0, 0, 0, 0 };
    double logSum = 0.0, linSum = 0.0;
    int flatBins = 0;

    for (size_t b = 1; b < power.size(); ++b)
    {
        const double hz = (double) b * binHz;
        const double p  = power[b];
        total += p;
        weighted += p * hz;
        bands[hz < 120.0 ? 0 : hz < 1000.0 ? 1 : hz < 5000.0 ? 2 : 3] += p;

        if (hz >= 40.0 && hz <= 16000.0)
        {
            logSum += std::log (p + 1.0e-12);
            linSum += p;
            ++flatBins;
        }
    }

    if (total <= 0.0)
        return f;

    const double centroid = weighted / total;
    double acc = 0.0, rolloff = 0.0;
    for (size_t b = 1; b < power.size(); ++b)
    {
        acc += power[b];
        if (acc >= 0.85 * total) { rolloff = (double) b * binHz; break; }
    }

    const double flatness = flatBins > 0 ? std::exp (logSum / flatBins) / (linSum / flatBins + 1.0e-12) : 0.0;

    // ---- zero crossings in the first 100 ms
    int crossings = 0;
    const int zEnd = juce::jmin (n, onset + (int) (0.1 * sr));
    for (int i = onset + 1; i < zEnd; ++i)
        if ((m[(size_t) i - 1] < 0.0f) != (m[(size_t) i] < 0.0f)) ++crossings;
    const double zcrHz = zEnd > onset + 1 ? crossings * sr / (double) (zEnd - onset) : 0.0;

    const float lo = std::log2 (50.0f), hi = std::log2 (16000.0f);
    f.centroidHz           = (float) centroid;
    f.v[fLogCentroid]      = norm01 (std::log2 ((float) juce::jmax (50.0, centroid)), lo, hi);
    f.v[fLogRolloff]       = norm01 (std::log2 ((float) juce::jmax (50.0, rolloff)), lo, hi);
    f.v[fFlatness]         = (float) juce::jlimit (0.0, 1.0, flatness);
    f.v[fLow]              = (float) (bands[0] / total);
    f.v[fLowMid]           = (float) (bands[1] / total);
    f.v[fHighMid]          = (float) (bands[2] / total);
    f.v[fHigh]             = (float) (bands[3] / total);
    f.v[fLogDecay]         = norm01 (std::log10 (f.decaySec), -2.5f, 0.6f);
    f.v[fLogAttack]        = norm01 (std::log10 (attackMs), -0.5f, 2.0f);
    f.v[fZcr]              = (float) juce::jlimit (0.0, 1.0, zcrHz / 10000.0);
    return f;
}

//==============================================================================
std::optional<Category> FeatureExtractor::classifyByName (const juce::File& file)
{
    if (auto c = matchName (file.getFileNameWithoutExtension()))
        return c;

    auto dir = file.getParentDirectory();
    for (int depth = 0; depth < 2 && dir != dir.getParentDirectory(); ++depth, dir = dir.getParentDirectory())
        if (auto c = matchName (dir.getFileName()))
            return c;

    return std::nullopt;
}

Category FeatureExtractor::classifyBySound (const SampleFeatures& f)
{
    const float low  = f.v[fLow];
    const float high = f.v[fHigh] + f.v[fHighMid] * 0.5f;
    const float flat = f.v[fFlatness];
    const float c    = f.centroidHz;

    if (c <= 0.0f)                                          return Category::Other;
    if (low > 0.45f && c < 500.0f)                          return Category::Kick;
    if (f.bursts >= 3 && flat > 0.2f && c > 500.0f && c < 6000.0f)
                                                            return Category::Clap;
    if (high > 0.5f && c > 5500.0f)                         return f.decaySec > 0.5f ? Category::Cymbal : Category::HiHat;
    if (flat > 0.10f && c > 700.0f)                         return Category::Snare;
    if (flat < 0.10f && c < 1200.0f && f.decaySec > 0.08f)  return low > 0.35f && c < 300.0f ? Category::Kick : Category::Tom;
    if (low > 0.35f && c < 700.0f)                          return Category::Kick;
    return Category::Perc;
}

Category FeatureExtractor::classify (const juce::File& file, const SampleFeatures& f)
{
    if (auto c = classifyByName (file))
        return *c;
    return classifyBySound (f);
}
} // namespace sk
