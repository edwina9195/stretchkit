#include "SampleLibrary.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <unordered_map>

namespace sk
{
namespace
{
constexpr int kCacheMagic   = 0x324c4b53; // "SKL2"
constexpr int kCacheVersion = 2;

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

/** Smoothly keeps a value inside [lo, hi] (no hard edges, so the cloud never piles up into lines). */
float softClamp (float v, float lo, float hi)
{
    const float mid = 0.5f * (lo + hi), half = 0.5f * (hi - lo);
    return mid + half * std::tanh ((v - mid) / half);
}

/** Analyses one file. Returns false for non-audio, unreadable or too-long files. */
bool analyseFile (juce::AudioFormatManager& formats, const juce::File& file, juce::int64 modTime, SampleEntry& out)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->sampleRate <= 0.0)
        return false;

    const double sr = reader->sampleRate;
    const float duration = (float) ((double) reader->lengthInSamples / sr);
    if (duration <= 0.01f || duration > SampleLibrary::maxOneShotSeconds)
        return false;

    const int n = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (4.0 * sr));
    juce::AudioBuffer<float> buf ((int) juce::jlimit (1u, 2u, reader->numChannels), n);
    reader->read (&buf, 0, n, 0, true, true);

    const auto features = FeatureExtractor::analyse (buf, sr, duration);
    out.path     = file.getFullPathName();
    out.name     = file.getFileNameWithoutExtension();
    out.category = FeatureExtractor::classify (file, features);
    out.features = features.v;
    out.duration = duration;
    out.modTime  = modTime;
    return true;
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
    stopThread (20000);
}

juce::File SampleLibrary::cacheDir()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #endif
    return dir.getChildFile ("StretchKit");
}

void SampleLibrary::addFolder (const juce::File& folder)
{
    {
        std::lock_guard<std::mutex> lock (mutex);
        folders.addIfNotAlreadyThere (folder.getFullPathName());
    }
    saveInBackground();   // the folder is remembered right away, before the scan even starts
    rescan();
}

void SampleLibrary::removeFolder (const juce::String& path)
{
    {
        std::lock_guard<std::mutex> lock (mutex);
        folders.removeString (path);
    }
    saveInBackground();
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

bool SampleLibrary::isFavourite (const juce::String& path) const
{
    std::lock_guard<std::mutex> lock (mutex);
    return favourites.count (path) > 0;
}

void SampleLibrary::setFavourite (const juce::String& path, bool shouldBeFavourite)
{
    {
        std::lock_guard<std::mutex> lock (mutex);
        if (shouldBeFavourite) favourites.insert (path);
        else                   favourites.erase (path);
    }
    sendChangeMessage();
    saveInBackground();
}

void SampleLibrary::saveInBackground()
{
    // the UI never waits for a large cache file; queued saves collapse into one
    if (saver.getNumJobs() < 2)
        saver.addJob ([this] { saveCacheFromSnapshot(); });
}

//==============================================================================
void SampleLibrary::run()
{
    loadCache();
    loaded = true;
    sendChangeMessage();

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

    // what we already know (kept alive by `old` for the whole scan)
    const auto old = getSnapshot();
    std::unordered_map<juce::String, const SampleEntry*> known;
    known.reserve (old->size());
    for (const auto& e : *old)
        known.emplace (e.path, &e);

    // 1) collect files
    std::vector<juce::File> files;
    for (const auto& folder : folderList)
    {
        for (const auto& entry : juce::RangedDirectoryIterator (juce::File (folder), true, "*", juce::File::findFiles))
        {
            if (threadShouldExit()) { scanning = false; return; }
            if (isAudioFile (entry.getFile()))
                files.push_back (entry.getFile());
            if (files.size() >= 400000)
                break;
        }
    }

    const size_t n = files.size();
    totalToScan = (int) n;

    // 2) analyse new / changed files on several cores
    std::vector<SampleEntry> results (n);
    std::unique_ptr<std::atomic<uint8_t>[]> state (new std::atomic<uint8_t>[n]);   // 0 todo, 1 have entry, 2 skip
    for (size_t i = 0; i < n; ++i)
        state[i] = 0;

    std::atomic<size_t> next { 0 };
    std::atomic<int> done { 0 }, analysed { 0 };

    auto worker = [&]
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();

        for (size_t i = next++; i < n && ! threadShouldExit(); i = next++)
        {
            const auto& file = files[i];
            const auto modTime = file.getLastModificationTime().toMilliseconds();
            const auto it = known.find (file.getFullPathName());

            if (it != known.end() && it->second->modTime == modTime)
            {
                results[i] = *it->second;
                state[i] = 1;
            }
            else
            {
                state[i] = analyseFile (formats, file, modTime, results[i]) ? 1 : 2;
                ++analysed;
            }
            ++done;
        }
    };

    const int numWorkers = juce::jlimit (1, 8, juce::SystemStats::getNumCpus() - 1);
    std::vector<std::thread> workers;
    for (int w = 0; w < numWorkers; ++w)
        workers.emplace_back (worker);

    // everything finished so far, plus what we knew about files that haven't been reached yet
    auto collect = [&]
    {
        std::vector<SampleEntry> out;
        out.reserve (n);
        for (size_t i = 0; i < n; ++i)
        {
            const auto st = state[i].load();
            if (st == 1)
                out.push_back (results[i]);
            else if (st == 0)
                if (const auto it = known.find (files[i].getFullPathName()); it != known.end())
                    out.push_back (*it->second);
        }
        return out;
    };

    int lastPublished = 0, lastSaved = 0;
    auto lastPublishTime = juce::Time::getMillisecondCounter();

    while (done.load() < (int) n && ! threadShouldExit())
    {
        wait (400);
        scannedCount = done.load();

        const int a = analysed.load();
        const int publishEvery = juce::jmax (2000, (int) n / 12);
        if (a - lastPublished >= publishEvery || (a > lastPublished && juce::Time::getMillisecondCounter() - lastPublishTime > 5000))
        {
            lastPublished = a;
            lastPublishTime = juce::Time::getMillisecondCounter();
            auto partial = collect();
            if (a - lastSaved >= 4000)
            {
                lastSaved = a;
                saveCache (partial, false);   // progress survives closing the plugin mid-scan
            }
            publish (std::move (partial));
        }
        else
        {
            sendChangeMessage();   // progress counter
        }
    }

    for (auto& t : workers)
        t.join();

    auto entries = collect();
    const bool complete = ! threadShouldExit();
    saveCache (entries, complete);

    if (complete)
        publish (std::move (entries));

    scannedCount = done.load();
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

    const double total = (double) entries.size();

    for (int ci = 0; ci < kNumCategories; ++ci)
    {
        const auto cat = (Category) ci;
        std::vector<SampleEntry*> group;
        for (auto& e : entries)
            if (e.category == cat)
                group.push_back (&e);

        if (group.empty())
            continue;

        const size_t m = group.size();
        std::vector<double> g1 (m, 0.0), g2 (m, 0.0);

        if (m >= 3)
        {
            // the two strongest directions of variation inside this category
            Vec gm {};
            for (auto* e : group)
                for (int k = 0; k < kNumFeatures; ++k)
                    gm[(size_t) k] += e->z[(size_t) k];
            for (auto& v : gm) v /= (double) m;

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

            std::vector<double> p1 (m), p2 (m);
            for (size_t i = 0; i < m; ++i)
            {
                double a = 0.0, b = 0.0;
                for (int k = 0; k < kNumFeatures; ++k)
                {
                    const double d = group[i]->z[(size_t) k] - gm[(size_t) k];
                    a += d * v1[(size_t) k];
                    b += d * v2[(size_t) k];
                }
                // tiny tie-breaker so identical sounds don't stack on one pixel
                p1[i] = a + 1.0e-4 * hashJitter (group[i]->path, 1);
                p2[i] = b + 1.0e-4 * hashJitter (group[i]->path, 2);
            }

            // ranks along the strongest direction ...
            std::vector<size_t> order (m);
            for (size_t i = 0; i < m; ++i) order[i] = i;
            std::sort (order.begin(), order.end(), [&p1] (size_t a, size_t b) { return p1[a] < p1[b]; });
            for (size_t r = 0; r < m; ++r)
                g1[order[r]] = ((double) r + 0.5) / (double) m;

            // ... and along the second one, inside narrow slices of the first: fills the cloud evenly
            // while sounds that are close in both directions stay close
            const size_t slice = (size_t) juce::jlimit (6.0, 400.0, std::sqrt ((double) m) * 1.3);
            for (size_t begin = 0; begin < m; begin += slice)
            {
                const size_t end = std::min (m, begin + slice);
                std::vector<size_t> bin (order.begin() + (long) begin, order.begin() + (long) end);
                std::sort (bin.begin(), bin.end(), [&p2] (size_t a, size_t b) { return p2[a] < p2[b]; });
                for (size_t r = 0; r < bin.size(); ++r)
                    g2[bin[r]] = ((double) r + 0.5) / (double) bin.size();
            }
        }
        else
        {
            for (size_t i = 0; i < m; ++i)
            {
                g1[i] = 0.5 + 0.3 * hashJitter (group[i]->path, 1);
                g2[i] = 0.5 + 0.3 * hashJitter (group[i]->path, 2);
            }
        }

        const auto centre = categoryCentre (cat);
        const float spread = 0.10f + 0.29f * (float) std::sqrt ((double) m / total);
        const float rx = spread * 1.55f, ry = spread * 1.05f;
        const float phase = 6.2832f * (0.5f + 0.5f * hashJitter (categoryName (cat), 3));
        const float tilt = 0.35f * hashJitter (categoryName (cat), 7);
        const float ct = std::cos (tilt), st = std::sin (tilt);
        const float grain = 0.55f / std::sqrt ((float) std::max<size_t> (1, m));   // about one dot spacing

        for (size_t i = 0; i < m; ++i)
        {
            // square of ranks -> disc (Shirley-Chiu concentric map keeps neighbours together)
            const float a = 2.0f * (float) g1[i] - 1.0f, b = 2.0f * (float) g2[i] - 1.0f;
            float rad, th;
            if (std::abs (a) > std::abs (b)) { rad = a; th = 0.7854f * (b / a); }
            else if (std::abs (b) > 1.0e-9f) { rad = b; th = 1.5708f - 0.7854f * (a / b); }
            else                             { rad = 0.0f; th = 0.0f; }

            // denser middle, soft blobby edge
            const float r0 = std::pow (std::abs (rad), 1.15f);
            const float edge = 1.0f + 0.13f * std::sin (3.0f * th + phase) + 0.07f * std::sin (5.0f * th + 2.1f * phase);
            const float fuzz = 1.0f + 0.22f * r0 * r0 * r0 * hashJitter (group[i]->path, 5);
            const float rr = (rad < 0.0f ? -1.0f : 1.0f) * r0 * edge * fuzz;

            float dx = rr * std::cos (th), dy = rr * std::sin (th);
            dx += grain * hashJitter (group[i]->path, 8);
            dy += grain * hashJitter (group[i]->path, 9);
            const float x = (dx * ct - dy * st) * rx;
            const float y = (dx * st + dy * ct) * ry;
            group[i]->x = softClamp (centre.x + x, 0.03f, kWorldW - 0.03f);
            group[i]->y = softClamp (centre.y + y, 0.03f, 0.97f);
        }
    }
}

//==============================================================================
std::vector<int> SampleLibrary::nearest (const std::vector<SampleEntry>& snap, int index, int k)
{
    std::vector<int> out;
    if (! juce::isPositiveAndBelow (index, (int) snap.size()) || k <= 0)
        return out;

    const auto& self = snap[(size_t) index];
    std::vector<std::pair<float, int>> cands;
    cands.reserve (snap.size());
    for (int i = 0; i < (int) snap.size(); ++i)
    {
        if (i == index)
            continue;
        const auto& e = snap[(size_t) i];
        float d = 0.0f;
        for (int f = 0; f < kNumFeatures; ++f)
            d += juce::square (e.z[(size_t) f] - self.z[(size_t) f]);
        if (e.category != self.category)
            d += 0.6f;                       // gently prefer the same kind of sound
        cands.emplace_back (d, i);
    }

    const size_t kk = std::min<size_t> ((size_t) k, cands.size());
    std::partial_sort (cands.begin(), cands.begin() + (long) kk, cands.end());
    for (size_t i = 0; i < kk; ++i)
        out.push_back (cands[i].second);
    return out;
}

juce::String SampleLibrary::findSimilar (const juce::String& path, juce::Random& rng) const
{
    const auto snap = getSnapshot();
    int self = -1;
    for (int i = 0; i < (int) snap->size(); ++i)
        if ((*snap)[(size_t) i].path == path) { self = i; break; }

    if (self < 0)
        return findRandom (categoryFor (path), rng);

    std::vector<int> sameCat;
    for (int idx : nearest (*snap, self, 40))
        if ((*snap)[(size_t) idx].category == (*snap)[(size_t) self].category)
            sameCat.push_back (idx);

    if (sameCat.empty())
        return {};
    const int pick = (int) std::min<size_t> (12, sameCat.size());
    return (*snap)[(size_t) sameCat[(size_t) rng.nextInt (pick)]].path;
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
    const auto bin = cacheDir().getChildFile ("library.bin");
    std::vector<SampleEntry> entries;
    bool complete = true;

    if (bin.existsAsFile())
    {
        juce::FileInputStream raw (bin);
        if (raw.openedOk())
        {
            juce::BufferedInputStream in (raw, 1 << 16);
            if (in.readInt() == kCacheMagic && in.readInt() == kCacheVersion)
            {
                complete = in.readInt() != 0;

                juce::StringArray f;
                for (int i = juce::jlimit (0, 10000, in.readInt()); --i >= 0;)
                    f.add (in.readString());

                std::set<juce::String> fav;
                for (int i = juce::jlimit (0, 10000000, in.readInt()); --i >= 0;)
                    fav.insert (in.readString());

                const int count = juce::jlimit (0, 10000000, in.readInt());
                entries.reserve ((size_t) count);
                for (int i = 0; i < count && ! in.isExhausted(); ++i)
                {
                    SampleEntry e;
                    e.path     = in.readString();
                    e.name     = juce::File (e.path).getFileNameWithoutExtension();
                    e.modTime  = in.readInt64();
                    e.category = (Category) juce::jlimit (0, kNumCategories - 1, (int) in.readByte());
                    e.duration = in.readFloat();
                    for (auto& v : e.features)
                        v = in.readFloat();
                    if (e.path.isNotEmpty())
                        entries.push_back (std::move (e));
                }

                std::lock_guard<std::mutex> lock (mutex);
                for (const auto& s : f)
                    folders.addIfNotAlreadyThere (s);
                favourites = std::move (fav);
            }
        }
    }
    else if (const auto json = cacheDir().getChildFile ("library.json"); json.existsAsFile())
    {
        // StretchKit 0.1.0 cache: take over the folder list, then rebuild
        const auto parsed = juce::JSON::parse (json);
        if (auto* folderArr = parsed["folders"].getArray())
        {
            std::lock_guard<std::mutex> lock (mutex);
            for (const auto& v : *folderArr)
                folders.addIfNotAlreadyThere (v.toString());
        }
        complete = false;
    }

    if (! entries.empty())
        publish (std::move (entries));

    if (! complete && ! getFolders().isEmpty())
        scanRequested = true;   // the last scan was interrupted: carry on where it stopped
}

void SampleLibrary::saveCacheFromSnapshot()
{
    const auto snap = getSnapshot();
    saveCache (*snap, ! isScanning() && ! scanRequested.load());
}

void SampleLibrary::saveCache (const std::vector<SampleEntry>& entries, bool complete)
{
    std::lock_guard<std::mutex> saveLock (saveMutex);

    juce::StringArray folderList;
    std::vector<juce::String> fav;
    {
        std::lock_guard<std::mutex> lock (mutex);
        folderList = folders;
        fav.assign (favourites.begin(), favourites.end());
    }

    const auto dir = cacheDir();
    dir.createDirectory();
    const auto target = dir.getChildFile ("library.bin");

    juce::TemporaryFile temp (target);
    {
        juce::FileOutputStream raw (temp.getFile(), 1 << 16);
        if (! raw.openedOk())
            return;

        raw.writeInt (kCacheMagic);
        raw.writeInt (kCacheVersion);
        raw.writeInt (complete ? 1 : 0);
        raw.writeInt (folderList.size());
        for (const auto& f : folderList)
            raw.writeString (f);
        raw.writeInt ((int) fav.size());
        for (const auto& f : fav)
            raw.writeString (f);
        raw.writeInt ((int) entries.size());
        for (const auto& e : entries)
        {
            raw.writeString (e.path);
            raw.writeInt64 (e.modTime);
            raw.writeByte ((char) e.category);
            raw.writeFloat (e.duration);
            for (auto v : e.features)
                raw.writeFloat (v);
        }
        raw.flush();
        if (raw.getStatus().failed())
            return;
    }
    temp.overwriteTargetFileWithTemporary();
}
} // namespace sk
