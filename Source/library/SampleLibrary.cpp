#include "SampleLibrary.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace sk
{
namespace
{
/** Deterministic pseudo-random value in [-1, 1] from a string, for jitter. */
float hashJitter (const juce::String& s, int salt)
{
    auto h = (juce::uint32) s.hashCode() ^ ((juce::uint32) salt * 0x9e3779b9u);
    h ^= h >> 16; h *= 0x7feb352d; h ^= h >> 15; h *= 0x846ca68b; h ^= h >> 16;
    return (float) (h & 0xffff) / 32767.5f - 1.0f;
}

using Mat = std::array<std::array<double, kNumFeatures>, kNumFeatures>;
using Vec = std::array<double, kNumFeatures>;

Vec topEigenvector (const Mat& m, double& eigenvalue)
{
    Vec v;
    for (int i = 0; i < kNumFeatures; ++i) v[(size_t) i] = 1.0 + 0.1 * i;

    for (int it = 0; it < 80; ++it)
    {
        Vec nv {};
        for (int r = 0; r < kNumFeatures; ++r)
            for (int c = 0; c < kNumFeatures; ++c)
                nv[(size_t) r] += m[(size_t) r][(size_t) c] * v[(size_t) c];

        double len = 0.0;
        for (auto x : nv) len += x * x;
        len = std::sqrt (len);
        if (len < 1.0e-12) { eigenvalue = 0.0; return v; }
        for (auto& x : nv) x /= len;
        v = nv;
        eigenvalue = len;
    }
    return v;
}
} // namespace

//==============================================================================
SampleLibrary::SampleLibrary() : juce::Thread ("StretchKit library")
{
    snapshot = std::make_shared<const std::vector<SampleEntry>>();
    startThread (juce::Thread::Priority::low);
}

SampleLibrary::~SampleLibrary()
{
    signalThreadShouldExit();
    notify();
    stopThread (10000);
}

juce::File SampleLibrary::cacheFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #endif
    return dir.getChildFile ("StretchKit").getChildFile ("library.json");
}

void SampleLibrary::addFolder (const juce::File& folder)
{
    {
        std::lock_guard<std::mutex> lock (mutex);
        folders.addIfNotAlreadyThere (folder.getFullPathName());
    }
    rescan();
}

void SampleLibrary::removeFolder (const juce::String& path)
{
    {
        std::lock_guard<std::mutex> lock (mutex);
        folders.removeString (path);
    }
    rescan();
}

juce::StringArray SampleLibrary::getFolders() const
{
    std::lock_guard<std::mutex> lock (mutex);
    return folders;
}

void SampleLibrary::rescan()
{
    scanRequested = true;
    notify();
}

LibrarySnapshot SampleLibrary::getSnapshot() const
{
    std::lock_guard<std::mutex> lock (mutex);
    return snapshot;
}

//==============================================================================
void SampleLibrary::run()
{
    loadCache();

    while (! threadShouldExit())
    {
        if (scanRequested.exchange (false))
            scanNow();
        else
            wait (-1);
    }
}

void SampleLibrary::scanNow()
{
    scanning = true;
    scannedCount = 0;
    totalToScan = 0;
    sendChangeMessage();

    const auto folderList = getFolders();

    std::unordered_map<juce::String, SampleEntry> known;
    for (const auto& e : *getSnapshot())
        known.emplace (e.path, e);

    // 1) collect files
    std::vector<juce::File> files;
    for (const auto& folder : folderList)
    {
        for (const auto& entry : juce::RangedDirectoryIterator (juce::File (folder), true, "*", juce::File::findFiles))
        {
            if (threadShouldExit()) { scanning = false; return; }
            if (isAudioFile (entry.getFile()))
                files.push_back (entry.getFile());
            if (files.size() >= 200000)
                break;
        }
    }
    totalToScan = (int) files.size();

    // 2) analyse new / changed files
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::vector<SampleEntry> entries;
    entries.reserve (files.size());

    for (size_t i = 0; i < files.size(); ++i)
    {
        if (threadShouldExit()) { scanning = false; return; }

        const auto& file = files[i];
        const auto path = file.getFullPathName();
        const auto modTime = file.getLastModificationTime().toMilliseconds();

        if (auto it = known.find (path); it != known.end() && it->second.modTime == modTime)
        {
            entries.push_back (it->second);
        }
        else if (std::unique_ptr<juce::AudioFormatReader> reader { formats.createReaderFor (file) })
        {
            const double sr = reader->sampleRate;
            const float duration = sr > 0.0 ? (float) ((double) reader->lengthInSamples / sr) : 0.0f;

            if (duration > 0.01f && duration <= maxOneShotSeconds)
            {
                const int n = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (6.0 * sr));
                juce::AudioBuffer<float> buf ((int) juce::jlimit (1u, 2u, reader->numChannels), n);
                reader->read (&buf, 0, n, 0, true, true);

                const auto features = FeatureExtractor::analyse (buf, sr, duration);

                SampleEntry e;
                e.path     = path;
                e.name     = file.getFileNameWithoutExtension();
                e.category = FeatureExtractor::classify (file, features);
                e.features = features.v;
                e.duration = duration;
                e.modTime  = modTime;
                entries.push_back (std::move (e));
            }
        }

        scannedCount = (int) i + 1;

        if ((i + 1) % 1500 == 0)   // let the map fill in while a big library scans
            publish (std::vector<SampleEntry> (entries));
    }

    saveCache (entries, folderList);
    publish (std::move (entries));
    scanning = false;
    sendChangeMessage();
}

void SampleLibrary::publish (std::vector<SampleEntry>&& entries)
{
    computeLayout (entries);
    {
        std::lock_guard<std::mutex> lock (mutex);
        snapshot = std::make_shared<const std::vector<SampleEntry>> (std::move (entries));
    }
    sendChangeMessage();
}

//==============================================================================
void SampleLibrary::computeLayout (std::vector<SampleEntry>& entries)
{
    if (entries.empty())
        return;

    // standardise each feature over the whole library
    Vec mean {}, sd {};
    for (const auto& e : entries)
        for (int k = 0; k < kNumFeatures; ++k)
            mean[(size_t) k] += e.features[(size_t) k];
    for (auto& m : mean) m /= (double) entries.size();

    for (const auto& e : entries)
        for (int k = 0; k < kNumFeatures; ++k)
            sd[(size_t) k] += juce::square (e.features[(size_t) k] - mean[(size_t) k]);
    for (auto& s : sd) s = std::sqrt (s / (double) entries.size()) + 1.0e-6;

    for (auto& e : entries)
        for (int k = 0; k < kNumFeatures; ++k)
            e.z[(size_t) k] = (float) ((e.features[(size_t) k] - mean[(size_t) k]) / sd[(size_t) k]);

    // per category: PCA to two dimensions, placed around that category's island centre
    for (int ci = 0; ci < kNumCategories; ++ci)
    {
        const auto cat = (Category) ci;
        std::vector<SampleEntry*> group;
        for (auto& e : entries)
            if (e.category == cat)
                group.push_back (&e);

        if (group.empty())
            continue;

        std::vector<float> p1 (group.size(), 0.0f), p2 (group.size(), 0.0f);

        if (group.size() >= 3)
        {
            Vec gm {};
            for (auto* e : group)
                for (int k = 0; k < kNumFeatures; ++k)
                    gm[(size_t) k] += e->z[(size_t) k];
            for (auto& m : gm) m /= (double) group.size();

            Mat cov {};
            for (auto* e : group)
                for (int r = 0; r < kNumFeatures; ++r)
                    for (int c = 0; c < kNumFeatures; ++c)
                        cov[(size_t) r][(size_t) c] += (e->z[(size_t) r] - gm[(size_t) r]) * (e->z[(size_t) c] - gm[(size_t) c]);

            double l1 = 0.0, l2 = 0.0;
            const auto v1 = topEigenvector (cov, l1);
            for (int r = 0; r < kNumFeatures; ++r)
                for (int c = 0; c < kNumFeatures; ++c)
                    cov[(size_t) r][(size_t) c] -= l1 * v1[(size_t) r] * v1[(size_t) c];
            const auto v2 = topEigenvector (cov, l2);

            for (size_t i = 0; i < group.size(); ++i)
            {
                double a = 0.0, b = 0.0;
                for (int k = 0; k < kNumFeatures; ++k)
                {
                    const double d = group[i]->z[(size_t) k] - gm[(size_t) k];
                    a += d * v1[(size_t) k];
                    b += d * v2[(size_t) k];
                }
                p1[i] = (float) a;
                p2[i] = (float) b;
            }

            auto scale95 = [] (std::vector<float> v)
            {
                for (auto& x : v) x = std::abs (x);
                const auto idx = (size_t) ((double) (v.size() - 1) * 0.95);
                std::nth_element (v.begin(), v.begin() + (long) idx, v.end());
                return juce::jmax (1.0e-3f, v[idx]);
            };
            const float s1 = scale95 (p1), s2 = scale95 (p2);
            for (size_t i = 0; i < group.size(); ++i)
            {
                p1[i] = juce::jlimit (-1.0f, 1.0f, p1[i] / s1);
                p2[i] = juce::jlimit (-1.0f, 1.0f, p2[i] / s2);
            }
        }

        const auto centre = categoryCentre (cat);
        const float jitter = group.size() < 3 ? 0.35f : 0.06f;
        for (size_t i = 0; i < group.size(); ++i)
        {
            auto* e = group[i];
            const float px = p1[i] + jitter * hashJitter (e->path, 1);
            const float py = p2[i] + jitter * hashJitter (e->path, 2);
            e->x = juce::jlimit (0.02f, 0.98f, centre.x + px * 0.14f);
            e->y = juce::jlimit (0.03f, 0.97f, centre.y + py * 0.13f);
        }
    }
}

//==============================================================================
juce::String SampleLibrary::findSimilar (const juce::String& path, juce::Random& rng) const
{
    const auto snap = getSnapshot();
    const SampleEntry* self = nullptr;
    for (const auto& e : *snap)
        if (e.path == path) { self = &e; break; }

    if (self == nullptr)
        return findRandom (categoryFor (path), rng);

    std::vector<std::pair<float, const SampleEntry*>> cands;
    for (const auto& e : *snap)
    {
        if (&e == self || e.category != self->category)
            continue;
        float d = 0.0f;
        for (int k = 0; k < kNumFeatures; ++k)
            d += juce::square (e.z[(size_t) k] - self->z[(size_t) k]);
        cands.emplace_back (d, &e);
    }

    if (cands.empty())
        return {};

    const size_t k = std::min<size_t> (12, cands.size());
    std::partial_sort (cands.begin(), cands.begin() + (long) k, cands.end(),
                       [] (const auto& a, const auto& b) { return a.first < b.first; });
    return cands[(size_t) rng.nextInt ((int) k)].second->path;
}

juce::String SampleLibrary::findRandom (Category c, juce::Random& rng) const
{
    const auto snap = getSnapshot();
    std::vector<const SampleEntry*> cands;
    for (const auto& e : *snap)
        if (e.category == c)
            cands.push_back (&e);

    if (cands.empty())
        return {};
    return cands[(size_t) rng.nextInt ((int) cands.size())]->path;
}

Category SampleLibrary::categoryFor (const juce::String& path) const
{
    for (const auto& e : *getSnapshot())
        if (e.path == path)
            return e.category;

    if (auto c = FeatureExtractor::classifyByName (juce::File (path)))
        return *c;
    return Category::Other;
}

//==============================================================================
void SampleLibrary::loadCache()
{
    const auto file = cacheFile();
    if (! file.existsAsFile())
        return;

    const auto json = juce::JSON::parse (file);
    std::vector<SampleEntry> entries;

    if (auto* folderArr = json["folders"].getArray())
    {
        std::lock_guard<std::mutex> lock (mutex);
        for (const auto& f : *folderArr)
            folders.addIfNotAlreadyThere (f.toString());
    }

    if (auto* arr = json["samples"].getArray())
    {
        entries.reserve ((size_t) arr->size());
        for (const auto& v : *arr)
        {
            SampleEntry e;
            e.path     = v["p"].toString();
            e.name     = juce::File (e.path).getFileNameWithoutExtension();
            e.modTime  = (juce::int64) v["m"];
            e.category = (Category) juce::jlimit (0, kNumCategories - 1, (int) v["c"]);
            e.duration = (float) (double) v["d"];
            if (auto* f = v["f"].getArray(); f != nullptr && f->size() == kNumFeatures)
                for (int k = 0; k < kNumFeatures; ++k)
                    e.features[(size_t) k] = (float) (double) f->getReference (k);
            if (e.path.isNotEmpty())
                entries.push_back (std::move (e));
        }
    }

    publish (std::move (entries));
}

void SampleLibrary::saveCache (const std::vector<SampleEntry>& entries, const juce::StringArray& folderList) const
{
    juce::Array<juce::var> samples;
    samples.ensureStorageAllocated ((int) entries.size());

    for (const auto& e : entries)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("p", e.path);
        o->setProperty ("m", e.modTime);
        o->setProperty ("c", (int) e.category);
        o->setProperty ("d", std::round (e.duration * 1000.0f) / 1000.0f);
        juce::Array<juce::var> f;
        for (auto x : e.features)
            f.add (std::round (x * 10000.0f) / 10000.0f);
        o->setProperty ("f", f);
        samples.add (juce::var (o));
    }

    juce::Array<juce::var> folderVars;
    for (const auto& f : folderList)
        folderVars.add (f);

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 1);
    root->setProperty ("folders", folderVars);
    root->setProperty ("samples", samples);

    const auto file = cacheFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (juce::JSON::toString (juce::var (root), true));
}
} // namespace sk
