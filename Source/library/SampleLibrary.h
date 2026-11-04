#pragma once

#include "Common.h"
#include "FeatureExtractor.h"

#include <juce_events/juce_events.h>

#include <memory>
#include <mutex>
#include <set>
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
    float x = 1.2f, y = 0.5f;                     // position on the map, world units (kWorldW x 1)
    juce::int64 modTime = 0;
};

using LibrarySnapshot = std::shared_ptr<const std::vector<SampleEntry>>;

/** Scans the user's drum folders on a background thread (using several CPU cores),
    classifies every one-shot and lays the results out as one organic cloud.
    Shared by every plugin instance (juce::SharedResourcePointer).

    Everything is remembered on disk: the folder list is saved the moment a folder is added,
    analysis results are saved every few thousand files during a scan, and a scan that was
    interrupted (plugin or DAW closed) continues by itself the next time StretchKit opens,
    skipping every file that was already analysed. */
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

    bool isScanning() const noexcept      { return scanning.load(); }
    bool isLoaded() const noexcept        { return loaded.load(); }
    int  getScannedCount() const noexcept { return scannedCount.load(); }
    int  getTotalToScan() const noexcept  { return totalToScan.load(); }

    LibrarySnapshot getSnapshot() const;

    /** Indices (into `snap`) of the k sounds closest to snap[index], any category, nearest first. */
    static std::vector<int> nearest (const std::vector<SampleEntry>& snap, int index, int k);

    /** A random sample close to `path` (same category). Empty if none. */
    juce::String findSimilar (const juce::String& path, juce::Random& rng) const;

    /** A random sample of the given category. Empty if none. */
    juce::String findRandom (Category c, juce::Random& rng) const;

    /** Category of a file: library entry if scanned, otherwise its name, otherwise Other. */
    Category categoryFor (const juce::String& path) const;

    bool isFavourite (const juce::String& path) const;
    void setFavourite (const juce::String& path, bool shouldBeFavourite);

    static constexpr float maxOneShotSeconds = 10.0f;

    /** Places entries on the map and fills their z-scores. Public for tests. */
    static void computeLayout (std::vector<SampleEntry>& entries);

private:
    void run() override;
    void scanNow();
    void loadCache();
    void saveCache (const std::vector<SampleEntry>&, bool complete);
    void saveCacheFromSnapshot();
    void saveInBackground();
    void publish (std::vector<SampleEntry>&& entries);
    static juce::File cacheDir();

    mutable std::mutex mutex;
    LibrarySnapshot snapshot;
    juce::StringArray folders;
    std::set<juce::String> favourites;

    std::mutex saveMutex;
    std::atomic<bool> scanRequested { false }, scanning { false }, loaded { false };
    std::atomic<int> scannedCount { 0 }, totalToScan { 0 };

    juce::ThreadPool saver { 1 };   // last member: finishes pending saves before anything else goes

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLibrary)
};
} // namespace sk
