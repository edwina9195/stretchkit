#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace sk
{
//==============================================================================
/** Loads sources and renders stretched versions in the background. */
class StretchKitProcessor::RenderThread : public juce::Thread
{
public:
    explicit RenderThread (StretchKitProcessor& p) : juce::Thread ("StretchKit render"), owner (p)
    {
        formats.registerBasicFormats();
        startThread (juce::Thread::Priority::normal);
    }

    ~RenderThread() override
    {
        signalThreadShouldExit();
        notify();
        stopThread (10000);
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            bool didWork = false;

            juce::String preview;
            {
                std::lock_guard<std::mutex> lock (owner.stateMutex);
                preview.swapWith (owner.previewRequest);
            }

            if (preview.isNotEmpty())
            {
                didWork = true;
                Source s;
                if (load (preview, s, 6.0))
                {
                    const double ratio = s.rate / owner.currentSampleRate.load();
                    owner.publishPreview (std::abs (ratio - 1.0) < 1.0e-6 ? std::move (s.buffer)
                                                                          : DrumStretcher::resample (s.buffer, ratio));
                }
            }

            for (int t = 0; t < kNumTracks && ! threadShouldExit(); ++t)
            {
                const int req = owner.requestVersion[(size_t) t].load();
                if (req == owner.renderedVersion[(size_t) t].load())
                    continue;

                didWork = true;

                juce::String path;
                {
                    std::lock_guard<std::mutex> lock (owner.stateMutex);
                    path = owner.trackPaths[(size_t) t];
                }

                auto& src = sources[(size_t) t];
                if (path != src.path)
                {
                    src = {};
                    src.path = path;
                    if (path.isNotEmpty())
                        load (path, src);
                }

                auto settings = owner.settingsFor (t);
                settings.sourceRate = src.rate;

                juce::AudioBuffer<float> out;
                if (src.buffer.getNumSamples() > 0)
                    out = DrumStretcher::render (src.buffer, settings);

                owner.publish (t, std::move (out));
                owner.renderedVersion[(size_t) t] = req;
            }

            if (! didWork)
                wait (250);
        }
    }

private:
    struct Source
    {
        juce::String path;
        juce::AudioBuffer<float> buffer;
        double rate = 44100.0;
    };

    bool load (const juce::String& path, Source& s, double maxSeconds = 20.0)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (path)));
        if (reader == nullptr || reader->sampleRate <= 0.0)
            return false;

        const auto n = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (maxSeconds * reader->sampleRate));
        s.buffer.setSize ((int) juce::jlimit (1u, 2u, reader->numChannels), n);
        reader->read (&s.buffer, 0, n, 0, true, true);
        s.rate = reader->sampleRate;
        return true;
    }

    StretchKitProcessor& owner;
    juce::AudioFormatManager formats;
    std::array<Source, kNumTracks> sources;
};

//==============================================================================
static juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "StretchKit";
    o.folderName          = "StretchKit";
    o.filenameSuffix      = "settings";
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

int StretchKitProcessor::loadDefaultSkin()
{
    juce::PropertiesFile file (settingsOptions());
    return juce::jlimit (0, 4, file.getIntValue ("defaultSkin", 3));
}

void StretchKitProcessor::saveDefaultSkin (int index)
{
    juce::PropertiesFile file (settingsOptions());
    if (file.getIntValue ("defaultSkin", -1) != index)
    {
        file.setValue ("defaultSkin", index);
        file.saveIfNeeded();
    }
}

static juce::String trackId (const char* base, int t) { return juce::String (base) + juce::String (t); }

juce::AudioProcessorValueTreeState::ParameterLayout StretchKitProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto dbAttr  = AudioParameterFloatAttributes().withLabel ("dB");
    auto pctAttr = AudioParameterFloatAttributes()
                       .withLabel ("%")
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; });
    auto stAttr  = AudioParameterFloatAttributes()
                       .withLabel ("st")
                       .withStringFromValueFunction ([] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " st"; });

    for (int t = 0; t < kNumTracks; ++t)
    {
        const auto name = "T" + String (t + 1) + " ";

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { trackId ("vol", t), 1 }, name + "Volume",
                                                           NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), 0.0f, dbAttr));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { trackId ("pan", t), 1 }, name + "Pan",
                                                           NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { trackId ("pitch", t), 1 }, name + "Pitch",
                                                           NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, stAttr));

        NormalisableRange<float> lenRange (25.0f, 400.0f, 1.0f);
        lenRange.setSkewForCentre (100.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { trackId ("len", t), 1 }, name + "Length",
                                                           lenRange, 100.0f, pctAttr));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { trackId ("mute", t), 1 }, name + "Mute", false));
    }

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Stretch Mode", modeNames(), 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, pctAttr));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Step Rate", rateNames(), 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "steps", 1 }, "Steps", stepCountNames(), 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Internal BPM",
                                                       NormalisableRange<float> (40.0f, 220.0f, 0.1f), 120.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "master", 1 }, "Master",
                                                       NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f, dbAttr));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "choke", 1 }, "Choke", true));
    return layout;
}

//==============================================================================
StretchKitProcessor::StretchKitProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int t = 0; t < kNumTracks; ++t)
    {
        pVol[(size_t) t]   = apvts.getRawParameterValue (trackId ("vol", t));
        pPan[(size_t) t]   = apvts.getRawParameterValue (trackId ("pan", t));
        pPitch[(size_t) t] = apvts.getRawParameterValue (trackId ("pitch", t));
        pLen[(size_t) t]   = apvts.getRawParameterValue (trackId ("len", t));
        pMute[(size_t) t]  = apvts.getRawParameterValue (trackId ("mute", t));
        trackCategories[(size_t) t] = defaultTrackCategory (t);
    }
    pMode   = apvts.getRawParameterValue ("mode");
    pSwing  = apvts.getRawParameterValue ("swing");
    pRate   = apvts.getRawParameterValue ("rate");
    pSteps  = apvts.getRawParameterValue ("steps");
    pBpm    = apvts.getRawParameterValue ("bpm");
    pMaster = apvts.getRawParameterValue ("master");
    pChoke  = apvts.getRawParameterValue ("choke");

    // A starter groove so something happens as soon as samples are on the tracks.
    for (int s = 0; s < 16; ++s)
    {
        if (s % 4 == 0)              setStep (0, s, s == 0 ? 2 : 1);   // kick
        if (s == 4 || s == 12)       setStep (1, s, 2);                // snare
        if (s % 2 == 0)              setStep (3, s, s % 4 == 2 ? 2 : 1); // hats
    }
    setStep (0, 10, 1);

    themeIndex = loadDefaultSkin();

    renderThread = std::make_unique<RenderThread> (*this);
    startTimerHz (20);
}

StretchKitProcessor::~StretchKitProcessor()
{
    stopTimer();
    renderThread.reset();
}

//==============================================================================
bool StretchKitProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void StretchKitProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& track : voices)
        for (auto& v : track)
            v = {};
    previewSample = nullptr;
    wasRunning = false;
    lastStepIndex = std::numeric_limits<juce::int64>::min();
    // the timer notices the new rate and re-renders every track
}

RenderSettings StretchKitProcessor::settingsFor (int t) const
{
    RenderSettings s;
    s.targetRate     = currentSampleRate.load();
    s.pitchSemitones = pPitch[(size_t) t]->load();
    s.lengthPercent  = pLen[(size_t) t]->load();
    s.mode           = (StretchMode) juce::jlimit (0, 2, (int) pMode->load());
    return s;
}

double StretchKitProcessor::stepBeats() const
{
    static const double beats[] = { 0.5, 0.25, 0.125, 1.0 / 6.0 };
    return beats[juce::jlimit (0, 3, (int) pRate->load())];
}

int StretchKitProcessor::getNumSteps() const
{
    static const int counts[] = { 8, 16, 32 };
    return counts[juce::jlimit (0, 2, (int) pSteps->load())];
}

void StretchKitProcessor::setInternalPlaying (bool shouldPlay) noexcept
{
    if (shouldPlay && ! internalPlaying.load())
        resetInternalClock = true;
    internalPlaying = shouldPlay;
}

void StretchKitProcessor::clearTrackSteps (int track) noexcept
{
    for (int s = 0; s < kMaxSteps; ++s)
        setStep (track, s, 0);
}

//==============================================================================
void StretchKitProcessor::setTrackSample (int track, const juce::String& path)
{
    if (! juce::isPositiveAndBelow (track, kNumTracks))
        return;

    const auto cat = path.isEmpty() ? defaultTrackCategory (track) : library->categoryFor (path);
    {
        std::lock_guard<std::mutex> lock (stateMutex);
        trackPaths[(size_t) track] = path;
        trackCategories[(size_t) track] = cat;
    }
    requestRender (track);
}

juce::String StretchKitProcessor::getTrackPath (int track) const
{
    std::lock_guard<std::mutex> lock (stateMutex);
    return trackPaths[(size_t) track];
}

juce::String StretchKitProcessor::getTrackName (int track) const
{
    const auto path = getTrackPath (track);
    if (path.isEmpty())
        return "(empty)";
    const juce::File f (path);
    return f.existsAsFile() ? f.getFileNameWithoutExtension() : "(missing) " + f.getFileNameWithoutExtension();
}

Category StretchKitProcessor::getTrackCategory (int track) const
{
    std::lock_guard<std::mutex> lock (stateMutex);
    return trackCategories[(size_t) track];
}

bool StretchKitProcessor::isTrackRendering (int track) const
{
    return requestVersion[(size_t) track].load() != renderedVersion[(size_t) track].load();
}

void StretchKitProcessor::randomiseTrack (int track)
{
    auto& rng = juce::Random::getSystemRandom();
    const auto current = getTrackPath (track);
    auto next = current.isNotEmpty() ? library->findSimilar (current, rng) : juce::String();
    if (next.isEmpty())
        next = library->findRandom (current.isNotEmpty() ? getTrackCategory (track) : defaultTrackCategory (track), rng);
    if (next.isNotEmpty())
        setTrackSample (track, next);
}

void StretchKitProcessor::previewFile (const juce::String& path)
{
    {
        std::lock_guard<std::mutex> lock (stateMutex);
        previewRequest = path;
    }
    if (renderThread != nullptr)
        renderThread->notify();
}

void StretchKitProcessor::requestRender (int track)
{
    ++requestVersion[(size_t) track];
    if (renderThread != nullptr)
        renderThread->notify();
}

//==============================================================================
RenderedSample* StretchKitProcessor::addToPool (juce::AudioBuffer<float>&& buffer)
{
    auto* s = new RenderedSample();
    s->buffer = std::move (buffer);
    std::lock_guard<std::mutex> lock (poolMutex);
    pool.add (s);
    return s;
}

void StretchKitProcessor::publish (int track, juce::AudioBuffer<float>&& buffer)
{
    auto* s = addToPool (std::move (buffer));
    if (auto* old = pending[(size_t) track].exchange (s))
        old->taken = true;   // superseded before the audio thread saw it
}

void StretchKitProcessor::publishPreview (juce::AudioBuffer<float>&& buffer)
{
    auto* s = addToPool (std::move (buffer));
    if (auto* old = pendingPreview.exchange (s))
        old->taken = true;
}

void StretchKitProcessor::checkRenderNeeded (int t)
{
    const auto s = settingsFor (t);
    auto& last = lastRequested[(size_t) t];
    const auto differs = [] (double a, double b) { return std::abs (a - b) > 1.0e-6; };
    if (differs (s.targetRate, last.targetRate) || differs (s.pitchSemitones, last.pitchSemitones)
        || differs (s.lengthPercent, last.lengthPercent) || s.mode != last.mode)
    {
        last = s;
        requestRender (t);
    }
}

void StretchKitProcessor::auditionTrack (int track)
{
    if (! juce::isPositiveAndBelow (track, kNumTracks))
        return;
    checkRenderNeeded (track);          // a slider that was just released may not have been picked up yet
    auditionPending[(size_t) track] = true;
}

void StretchKitProcessor::timerCallback()
{
    // Re-render a track when its pitch / length / mode or the host sample rate changes.
    for (int t = 0; t < kNumTracks; ++t)
        checkRenderNeeded (t);

    // Fresh instance + scanned library: load a random starter kit.
    if (freshInstance)
    {
        const auto snap = library->getSnapshot();
        bool allEmpty = true;
        for (int t = 0; t < kNumTracks; ++t)
            allEmpty = allEmpty && getTrackPath (t).isEmpty();

        if (! allEmpty)
            freshInstance = false;
        else if (! snap->empty())
        {
            freshInstance = false;
            for (int t = 0; t < kNumTracks; ++t)
                randomiseTrack (t);
        }
    }

    // Free rendered buffers nobody uses any more (never on the audio thread).
    std::lock_guard<std::mutex> lock (poolMutex);
    for (int i = pool.size(); --i >= 0;)
    {
        auto* s = pool.getObjectPointerUnchecked (i);
        if (s->taken.load() && s->getReferenceCount() == 1)
            pool.remove (i);
    }
}

//==============================================================================
void StretchKitProcessor::trigger (int track, float velocity)
{
    if (pMute[(size_t) track]->load() > 0.5f)
        return;

    auto& sample = active[(size_t) track];
    if (sample == nullptr || sample->buffer.getNumSamples() == 0)
        return;

    const bool choke = pChoke->load() > 0.5f;
    Voice* slot = nullptr;
    Voice* oldest = nullptr;

    for (auto& v : voices[(size_t) track])
    {
        if (v.active)
        {
            if (choke && v.fadeLeft < 0)
                v.fadeLeft = kChokeSamples;
            if (oldest == nullptr || v.pos > oldest->pos)
                oldest = &v;
        }
        else if (slot == nullptr)
        {
            slot = &v;
        }
    }

    if (slot == nullptr)
        slot = oldest;

    slot->sample   = sample;
    slot->pos      = 0;
    slot->velocity = velocity;
    slot->fadeLeft = -1;
    slot->active   = true;
    ++hitCounter[(size_t) track];
}

void StretchKitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // pick up newly rendered samples
    for (int t = 0; t < kNumTracks; ++t)
        if (auto* p = pending[(size_t) t].exchange (nullptr))
        {
            active[(size_t) t] = p;
            p->taken = true;
        }

    if (auto* p = pendingPreview.exchange (nullptr))
    {
        // fade the previous preview out instead of cutting it (no clicks while scrubbing the map)
        if (previewSample != nullptr && previewPos < previewSample->buffer.getNumSamples())
        {
            previewOld = previewSample;
            previewOldPos = previewPos;
            previewOldFade = kPreviewFade;
        }
        previewSample = p;
        p->taken = true;
        previewPos = 0;
    }

    int numEvents = 0;
    auto addEvent = [&] (int offset, int track, float vel)
    {
        if (numEvents < kMaxEvents)
            events[(size_t) numEvents++] = { offset, track, vel };
    };

    // track auditions from the editor, once the track's newest render is in place
    for (int t = 0; t < kNumTracks; ++t)
        if (auditionPending[(size_t) t].load()
            && requestVersion[(size_t) t].load() == renderedVersion[(size_t) t].load()
            && pending[(size_t) t].load() == nullptr)
        {
            auditionPending[(size_t) t] = false;
            addEvent (0, t, 1.0f);
        }

    // MIDI: C1.. triggers tracks
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const int t = m.getNoteNumber() - kFirstNote;
            if (juce::isPositiveAndBelow (t, kNumTracks))
                addEvent (juce::jlimit (0, n - 1, meta.samplePosition), t, m.getFloatVelocity());
        }
    }
    midi.clear();

    // transport
    const double sr = currentSampleRate.load();
    double bpm = pBpm->load();
    bool hostRun = false;
    double ppqStart = 0.0;

    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            if (const auto b = pos->getBpm())
                bpm = *b;
            if (pos->getIsPlaying())
                if (const auto ppq = pos->getPpqPosition())
                {
                    hostRun = true;
                    ppqStart = *ppq;
                }
        }

    bpm = juce::jlimit (20.0, 999.0, bpm);
    hostPlaying = hostRun;
    displayBpm = bpm;

    if (resetInternalClock.exchange (false))
        internalPpq = 0.0;

    const bool internalRun = ! hostRun && internalPlaying.load();
    const bool running = hostRun || internalRun;
    const double beatsPerSample = bpm / 60.0 / sr;

    if (internalRun)
        ppqStart = internalPpq;

    const double ppqEnd = ppqStart + n * beatsPerSample;

    if (internalRun)
        internalPpq = ppqEnd;

    if (running && n > 0)
    {
        if (! wasRunning || ppqStart < lastPpqEnd - 1.0e-4 || ppqStart > lastPpqEnd + 0.25)
            lastStepIndex = std::numeric_limits<juce::int64>::min();   // started, looped or jumped

        const double sb = stepBeats();
        const int len = getNumSteps();
        const double swingOffset = pSwing->load() / 100.0 * sb * 0.5;
        const double tolerance = 2.0 * beatsPerSample;

        const auto kFirst = (juce::int64) std::floor ((ppqStart - swingOffset - tolerance) / sb);
        const auto kLast  = (juce::int64) std::floor (ppqEnd / sb);

        for (auto k = kFirst; k <= kLast; ++k)
        {
            if (k < 0 || k <= lastStepIndex)
                continue;

            const double tk = (double) k * sb + ((k & 1) != 0 ? swingOffset : 0.0);
            if (tk < ppqStart - tolerance || tk >= ppqEnd)
                continue;

            // first sample at or after the step time
            const int offset = juce::jmax (0, (int) std::ceil ((tk - ppqStart) / beatsPerSample - 1.0e-6));
            if (offset >= n)
                break;   // falls on the very end of this block: handled by the next one
            const int stepIdx = (int) (k % len);

            for (int t = 0; t < kNumTracks; ++t)
                if (const int v = steps[(size_t) (t * kMaxSteps + stepIdx)].load())
                    addEvent (offset, t, v == 2 ? 1.0f : 0.72f);

            lastStepIndex = k;
        }

        currentStep = ppqStart >= 0.0 ? (int) (((juce::int64) std::floor (ppqStart / sb)) % len) : -1;
        lastPpqEnd = ppqEnd;
    }
    else
    {
        currentStep = -1;
    }
    wasRunning = running;

    std::sort (events.begin(), events.begin() + numEvents,
               [] (const Event& a, const Event& b) { return a.offset < b.offset; });

    // per-track gains (equal-power pan, unity at centre)
    const float master = juce::Decibels::decibelsToGain (pMaster->load());
    std::array<float, kNumTracks> gL {}, gR {};
    for (int t = 0; t < kNumTracks; ++t)
    {
        const float g = juce::Decibels::decibelsToGain (pVol[(size_t) t]->load(), -59.9f) * master;
        const float angle = (pPan[(size_t) t]->load() + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        gL[(size_t) t] = g * std::cos (angle) * juce::MathConstants<float>::sqrt2;
        gR[(size_t) t] = g * std::sin (angle) * juce::MathConstants<float>::sqrt2;
    }

    float* outL = buffer.getNumChannels() > 0 ? buffer.getWritePointer (0) : nullptr;
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    if (outL == nullptr)
        return;

    int ev = 0;
    for (int i = 0; i < n; ++i)
    {
        while (ev < numEvents && events[(size_t) ev].offset <= i)
        {
            trigger (events[(size_t) ev].track, events[(size_t) ev].velocity);
            ++ev;
        }

        float l = 0.0f, r = 0.0f;

        for (int t = 0; t < kNumTracks; ++t)
        {
            for (auto& v : voices[(size_t) t])
            {
                if (! v.active)
                    continue;

                const auto& b = v.sample->buffer;
                if (v.pos >= b.getNumSamples())
                {
                    v.active = false;
                    continue;
                }

                float g = v.velocity;
                if (v.fadeLeft >= 0)
                {
                    g *= (float) v.fadeLeft / (float) kChokeSamples;
                    if (--v.fadeLeft < 0)
                        v.active = false;
                }

                const float s0 = b.getSample (0, v.pos);
                const float s1 = b.getNumChannels() > 1 ? b.getSample (1, v.pos) : s0;
                l += s0 * g * gL[(size_t) t];
                r += s1 * g * gR[(size_t) t];
                ++v.pos;
            }
        }

        if (previewOld != nullptr && previewOldFade > 0 && previewOldPos < previewOld->buffer.getNumSamples())
        {
            const auto& b = previewOld->buffer;
            const float g = 0.8f * master * (float) previewOldFade / (float) kPreviewFade;
            const float s0 = b.getSample (0, previewOldPos);
            const float s1 = b.getNumChannels() > 1 ? b.getSample (1, previewOldPos) : s0;
            l += s0 * g;
            r += s1 * g;
            ++previewOldPos;
            --previewOldFade;
        }

        if (previewSample != nullptr && previewPos < previewSample->buffer.getNumSamples())
        {
            const auto& b = previewSample->buffer;
            const float s0 = b.getSample (0, previewPos);
            const float s1 = b.getNumChannels() > 1 ? b.getSample (1, previewPos) : s0;
            l += s0 * 0.8f * master;
            r += s1 * 0.8f * master;
            ++previewPos;
        }

        if (outR != nullptr)
        {
            outL[i] = l;
            outR[i] = r;
        }
        else
        {
            outL[i] = 0.5f * (l + r);
        }
    }

    // drop references held by finished voices (the pool still owns them, so nothing is freed here)
    for (auto& track : voices)
        for (auto& v : track)
            if (! v.active && v.sample != nullptr)
                v.sample = nullptr;

    if (previewSample != nullptr && previewPos >= previewSample->buffer.getNumSamples())
        previewSample = nullptr;
    if (previewOld != nullptr && (previewOldFade <= 0 || previewOldPos >= previewOld->buffer.getNumSamples()))
        previewOld = nullptr;
}

//==============================================================================
juce::File StretchKitProcessor::exportPatternAsMidi() const
{
    constexpr int tpq = 960;
    const double sb = stepBeats();
    const int len = getNumSteps();
    const double swingOffset = pSwing->load() / 100.0 * sb * 0.5;

    juce::MidiMessageSequence seq;
    for (int s = 0; s < len; ++s)
    {
        const double beat = s * sb + ((s & 1) != 0 ? swingOffset : 0.0);
        const double tick = beat * tpq;
        for (int t = 0; t < kNumTracks; ++t)
            if (const int v = getStep (t, s))
            {
                const auto vel = (juce::uint8) (v == 2 ? 127 : 92);
                seq.addEvent (juce::MidiMessage::noteOn (10, kFirstNote + t, vel), tick);
                seq.addEvent (juce::MidiMessage::noteOff (10, kFirstNote + t), tick + sb * tpq * 0.5);
            }
    }
    seq.addEvent (juce::MidiMessage::endOfTrack(), len * sb * tpq);
    seq.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("StretchKit Pattern.mid");
    file.deleteFile();
    if (auto out = file.createOutputStream())
        mf.writeTo (*out);
    return file;
}

//==============================================================================
void StretchKitProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::String data;
    for (const auto& s : steps)
        data << juce::String::charToString ((juce::juce_wchar) ('0' + s.load()));

    juce::ValueTree pattern ("PATTERN");
    pattern.setProperty ("data", data, nullptr);
    state.appendChild (pattern, nullptr);

    state.setProperty ("theme", themeIndex.load(), nullptr);
    state.setProperty ("editorW", editorW.load(), nullptr);
    state.setProperty ("editorH", editorH.load(), nullptr);

    juce::ValueTree tracks ("TRACKS");
    for (int t = 0; t < kNumTracks; ++t)
    {
        juce::ValueTree tr ("TRACK");
        tr.setProperty ("index", t, nullptr);
        tr.setProperty ("path", getTrackPath (t), nullptr);
        tracks.appendChild (tr, nullptr);
    }
    state.appendChild (tracks, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void StretchKitProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasType (apvts.state.getType()))
        return;

    freshInstance = false;

    if (tree.hasProperty ("theme"))
    {
        themeIndex = juce::jlimit (0, 4, (int) tree["theme"]);
        tree.removeProperty ("theme", nullptr);
    }
    editorW = (int) tree.getProperty ("editorW", 0);
    editorH = (int) tree.getProperty ("editorH", 0);
    tree.removeProperty ("editorW", nullptr);
    tree.removeProperty ("editorH", nullptr);

    if (auto pattern = tree.getChildWithName ("PATTERN"); pattern.isValid())
    {
        const auto str = pattern["data"].toString();
        for (int i = 0; i < (int) steps.size(); ++i)
            steps[(size_t) i] = (uint8_t) (i < str.length() ? juce::jlimit (0, 2, (int) (str[i] - '0')) : 0);
        tree.removeChild (pattern, nullptr);
    }

    if (auto tracks = tree.getChildWithName ("TRACKS"); tracks.isValid())
    {
        for (const auto& tr : tracks)
            setTrackSample ((int) tr["index"], tr["path"].toString());
        tree.removeChild (tracks, nullptr);
    }

    apvts.replaceState (tree);
}
} // namespace sk

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sk::StretchKitProcessor();
}

juce::AudioProcessorEditor* sk::StretchKitProcessor::createEditor()
{
    return new StretchKitEditor (*this);
}
