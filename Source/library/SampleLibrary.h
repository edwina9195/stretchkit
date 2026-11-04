#pragma once

#include "Common.h"
#include "FeatureExtractor.h"

#include <juce_events/juce_events.h>

#include <memory>
#include <mutex>
#include <vector>

namespace sk
{
struct SampleEntry
{
    juce::String path;
    juce::String name;
    Category category = Category::Other;
    std::array<float, kNumFeatures> features {};  // raw features
    std::array<float, kNumFeatures> z {};         // standardised (for similarity)
    float duration = 0.0f;
    float x = 0.5f, y = 0.5f;                     // position on the 0..1 map
    juce::int64 modTime = 0;
};

using LibrarySnapshot = std::shared_ptr<const std::vector<SampleEntry>>;

/** Scans the user's drum folders on a background thread, classifies every
    one-shot and lays the results out on a 2D map. Shared by every plugin
    instance (use juce::SharedResourcePointer) and cached on disk. */
class SampleLibrary : public juce::ChangeBroadcaster,
                      private juce::Thread
{
public:
    SampleLibrary();
    ~SampleLibrary() override;

    void addFolder (const juce::File& folder);
    void removeFolder (const juce::String& path);
    juce::StringArray getFolders() const;
    void rescan();

    bool isScanning() const noexcept   { return scanning.load(); }
    int  getScannedCount() const noexcept { return scannedCount.load(); }
    int  getTotalToScan() const noexcept  { return totalToScan.load(); }

    LibrarySnapshot getSnapshot() const;

    /** A random sample close to `path` on the map (same category). Empty if none. */
    juce::String findSimilar (const juce::String& path, juce::Random& rng) const;

    /** A random sample of the given category. Empty if none. */
    juce::String findRandom (Category c, juce::Random& rng) const;

    /** Category of a file: library entry if scanned, otherwise its name, otherwise Other. */
    Category categoryFor (const juce::String& path) const;

    static constexpr float maxOneShotSeconds = 10.0f;

private:
    void run() override;
    void scanNow();
    void loadCache();
    void saveCache (const std::vector<SampleEntry>&, const juce::StringArray& folderList) const;
    void publish (std::vector<SampleEntry>&& entries);
    static void computeLayout (std::vector<SampleEntry>& entries);
    static juce::File cacheFile();

    mutable std::mutex mutex;
    LibrarySnapshot snapshot;
    juce::StringArray folders;

    std::atomic<bool> scanRequested { false }, scanning { false };
    std::atomic<int> scannedCount { 0 }, totalToScan { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLibrary)
};
} // namespace sk
