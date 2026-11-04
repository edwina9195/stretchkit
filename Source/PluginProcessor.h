#pragma once

#include "Common.h"
#include "dsp/DrumStretcher.h"
#include "library/SampleLibrary.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <mutex>

namespace sk
{
/** A rendered (pitched + stretched + host-rate) sample. Lifetime is managed so
    the audio thread never allocates or frees: the processor's pool holds a
    reference until the audio thread has taken over (see processBlock / purge). */
struct RenderedSample : public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<RenderedSample>;
    juce::AudioBuffer<float> buffer;
    std::atomic<bool> taken { false };
};

class StretchKitProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    StretchKitProcessor();
    ~StretchKitProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==============================================================================
    // Editor API (message thread)
    juce::AudioProcessorValueTreeState apvts;
    juce::SharedResourcePointer<SampleLibrary> library;

    void setTrackSample (int track, const juce::String& path);
    juce::String getTrackPath (int track) const;
    juce::String getTrackName (int track) const;
    Category getTrackCategory (int track) const;
    bool isTrackRendering (int track) const;
    void randomiseTrack (int track);

    void previewFile (const juce::String& path);

    /** Plays the track's own (pitched + stretched) sound once, after any pending re-render. */
    void auditionTrack (int track);

    int  getEditorWidth() const noexcept  { return editorW.load(); }
    int  getEditorHeight() const noexcept { return editorH.load(); }
    void setEditorSize (int w, int h) noexcept { editorW = w; editorH = h; }

    int  getStep (int track, int step) const noexcept { return steps[(size_t) (track * kMaxSteps + step)].load(); }
    void setStep (int track, int step, int value) noexcept { steps[(size_t) (track * kMaxSteps + step)] = (uint8_t) value; }
    void clearTrackSteps (int track) noexcept;

    int  getNumSteps() const;
    int  getCurrentStep() const noexcept { return currentStep.load(); }
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }
    bool isInternalPlaying() const noexcept { return internalPlaying.load(); }
    void setInternalPlaying (bool shouldPlay) noexcept;
    double getDisplayBpm() const noexcept { return displayBpm.load(); }
    int  getHitCounter (int track) const noexcept { return hitCounter[(size_t) track].load(); }

    int  getThemeIndex() const noexcept { return themeIndex.load(); }

    /** The skin new instances start with (the last one picked, stored in the user's settings). */
    static int  loadDefaultSkin();
    static void saveDefaultSkin (int index);
    void setThemeIndex (int index) noexcept { themeIndex = index; }

    /** Writes the current pattern to a .mid file (for dragging into the DAW). */
    juce::File exportPatternAsMidi() const;

    static juce::StringArray rateNames() { return { "1/8", "1/16", "1/32", "1/16T" }; }
    static juce::StringArray stepCountNames() { return { "8", "16", "32" }; }
    static juce::StringArray modeNames() { return { "Hybrid", "Smooth", "Tape" }; }

private:
    //==============================================================================
    class RenderThread;
    friend class RenderThread;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void requestRender (int track);
    void publish (int track, juce::AudioBuffer<float>&& buffer);
    void publishPreview (juce::AudioBuffer<float>&& buffer);
    RenderedSample* addToPool (juce::AudioBuffer<float>&& buffer);
    double stepBeats() const;
    RenderSettings settingsFor (int track) const;
    void checkRenderNeeded (int track);

    struct Voice
    {
        RenderedSample::Ptr sample;
        int pos = 0;
        float velocity = 1.0f;
        int fadeLeft = -1;      // >= 0 while choking
        bool active = false;
    };

    static constexpr int kVoicesPerTrack = 4;
    static constexpr int kChokeSamples   = 220;
    static constexpr int kMaxEvents      = 512;

    struct Event { int offset; int track; float velocity; };

    void trigger (int track, float velocity);

    //==============================================================================
    // Message-thread / render-thread state
    mutable std::mutex stateMutex;
    std::array<juce::String, kNumTracks> trackPaths;
    std::array<Category, kNumTracks> trackCategories {};
    bool freshInstance = true;
    std::array<std::atomic<int>, kNumTracks> requestVersion {};
    std::array<std::atomic<int>, kNumTracks> renderedVersion {};
    std::array<RenderSettings, kNumTracks> lastRequested {};
    juce::String previewRequest;

    std::mutex poolMutex;
    juce::ReferenceCountedArray<RenderedSample> pool;

    std::unique_ptr<RenderThread> renderThread;
    std::atomic<double> currentSampleRate { 44100.0 };

    // Shared with the audio thread (lock-free)
    std::array<std::atomic<RenderedSample*>, kNumTracks> pending {};
    std::atomic<RenderedSample*> pendingPreview { nullptr };
    std::array<std::atomic<uint8_t>, kNumTracks * kMaxSteps> steps {};
    std::array<std::atomic<int>, kNumTracks> hitCounter {};
    std::atomic<int> currentStep { -1 };
    std::atomic<bool> hostPlaying { false }, internalPlaying { false }, resetInternalClock { false };
    std::atomic<double> displayBpm { 120.0 };
    std::array<std::atomic<bool>, kNumTracks> auditionPending {};
    std::atomic<int> editorW { 0 }, editorH { 0 };
    std::atomic<int> themeIndex { 3 };   // 0 PS2 Night, 1 Y2K Chrome, 2 Digicore, 3 Mix, 4 Liquid Chrome

    // Audio-thread only
    std::array<RenderedSample::Ptr, kNumTracks> active;
    std::array<std::array<Voice, kVoicesPerTrack>, kNumTracks> voices;
    RenderedSample::Ptr previewSample, previewOld;
    int previewPos = 0, previewOldPos = 0, previewOldFade = 0;
    static constexpr int kPreviewFade = 256;
    double internalPpq = 0.0;
    double lastPpqEnd = -1.0;
    juce::int64 lastStepIndex = std::numeric_limits<juce::int64>::min();
    bool wasRunning = false;
    std::array<Event, kMaxEvents> events {};

    // Cached parameter pointers
    std::array<std::atomic<float>*, kNumTracks> pVol {}, pPan {}, pPitch {}, pLen {}, pMute {};
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pSteps = nullptr;
    std::atomic<float>* pBpm = nullptr;
    std::atomic<float>* pMaster = nullptr;
    std::atomic<float>* pChoke = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StretchKitProcessor)
};
} // namespace sk
