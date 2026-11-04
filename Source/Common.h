#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

namespace sk
{
constexpr int kNumTracks    = 8;
constexpr int kMaxSteps     = 32;
constexpr int kFirstNote    = 36;   // C1 triggers track 1, C#1 track 2, ...
constexpr int kNumFeatures  = 10;

enum class Category : int
{
    Kick = 0, Snare, Clap, HiHat, Cymbal, Tom, Perc, Other,
    NumCategories
};

constexpr int kNumCategories = (int) Category::NumCategories;

inline const char* categoryName (Category c)
{
    switch (c)
    {
        case Category::Kick:   return "Kick";
        case Category::Snare:  return "Snare";
        case Category::Clap:   return "Clap";
        case Category::HiHat:  return "Hat";
        case Category::Cymbal: return "Cymbal";
        case Category::Tom:    return "Tom";
        case Category::Perc:   return "Perc";
        case Category::Other:
        case Category::NumCategories:
        default:               return "Other";
    }
}

inline juce::uint32 categoryArgb (Category c)
{
    switch (c)
    {
        case Category::Kick:   return 0xffff5a5f;
        case Category::Snare:  return 0xffffb347;
        case Category::Clap:   return 0xfff5e663;
        case Category::HiHat:  return 0xff5ee3c4;
        case Category::Cymbal: return 0xff5aa9ff;
        case Category::Tom:    return 0xffb98bff;
        case Category::Perc:   return 0xffff7ad9;
        case Category::Other:
        case Category::NumCategories:
        default:               return 0xff9aa3ad;
    }
}

/** Centre of each category's island on the 0..1 map. */
struct MapPoint { float x, y; };

inline MapPoint categoryCentre (Category c)
{
    switch (c)
    {
        case Category::Kick:   return { 0.18f, 0.22f };
        case Category::Snare:  return { 0.50f, 0.20f };
        case Category::Clap:   return { 0.82f, 0.22f };
        case Category::Tom:    return { 0.18f, 0.55f };
        case Category::Perc:   return { 0.50f, 0.55f };
        case Category::HiHat:  return { 0.82f, 0.55f };
        case Category::Other:  return { 0.30f, 0.85f };
        case Category::Cymbal: return { 0.72f, 0.85f };
        case Category::NumCategories:
        default:               return { 0.5f, 0.5f };
    }
}

/** Default category for each sequencer track (used by the dice button on empty tracks). */
inline Category defaultTrackCategory (int track)
{
    static const Category cats[kNumTracks] = { Category::Kick, Category::Snare, Category::Clap, Category::HiHat,
                                               Category::HiHat, Category::Tom, Category::Perc, Category::Cymbal };
    return cats[juce::jlimit (0, kNumTracks - 1, track)];
}

inline bool isAudioFile (const juce::File& f)
{
    return f.hasFileExtension ("wav;aif;aiff;flac;ogg");
}
} // namespace sk
