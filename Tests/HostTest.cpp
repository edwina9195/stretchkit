// End-to-end check of the processor without a DAW:
// library scan -> classification -> track loading -> sequencer timing -> length % -> state -> editor snapshot.
// Build with -DSTRETCHKIT_BUILD_TESTS=ON, run: StretchKitHostTest <work-folder>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <iostream>

using namespace sk;

namespace
{
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << "\n";
    if (! ok) ++failures;
}

void pump (int ms)
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil (ms);
}

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& b, double rate)
{
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (file.createOutputStream().release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (rate)
                                                                          .withNumChannels (b.getNumChannels())
                                                                          .withBitsPerSample (24));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

juce::AudioBuffer<float> kick (double sr)
{
    juce::AudioBuffer<float> b (1, (int) (0.5 * sr));
    double ph = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr;
        ph += juce::MathConstants<double>::twoPi * (45.0 + 110.0 * std::exp (-t * 30.0)) / sr;
        b.setSample (0, i, (float) (std::sin (ph) * std::exp (-t * 7.0) * 0.9));
    }
    return b;
}

juce::AudioBuffer<float> noise (double sr, double seconds, double decay, float lpCoeff, juce::Random& r)
{
    juce::AudioBuffer<float> b (1, (int) (seconds * sr));
    float lp = 0.0f, prev = 0.0f, hp = 0.0f;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float x = r.nextFloat() * 2.0f - 1.0f;
        lp += lpCoeff * (x - lp);
        hp = 0.9f * (hp + lp - prev);
        prev = lp;
        b.setSample (0, i, hp * (float) std::exp (-(i / sr) * decay));
    }
    return b;
}

/** Number of samples until the output stays below -60 dB. */
int audibleLength (const juce::AudioBuffer<float>& b)
{
    for (int i = b.getNumSamples(); --i >= 0;)
        if (std::abs (b.getSample (0, i)) > 0.001f)
            return i + 1;
    return 0;
}

void setParam (StretchKitProcessor& p, const juce::String& id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

juce::AudioBuffer<float> render (StretchKitProcessor& p, int numSamples, int block, juce::MidiBuffer* firstBlockMidi = nullptr)
{
    juce::AudioBuffer<float> out (2, numSamples);
    juce::AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < numSamples; pos += block)
    {
        juce::MidiBuffer midi;
        if (pos == 0 && firstBlockMidi != nullptr)
            midi = *firstBlockMidi;
        const int n = juce::jmin (block, numSamples - pos);
        buf.setSize (2, n, false, false, true);
        p.processBlock (buf, midi);
        out.copyFrom (0, pos, buf, 0, 0, n);
        out.copyFrom (1, pos, buf, 1, 0, n);
    }
    return out;
}

void waitForRenders (StretchKitProcessor& p)
{
    for (int i = 0; i < 400; ++i)
    {
        pump (10);
        bool busy = false;
        for (int t = 0; t < kNumTracks; ++t)
            busy = busy || p.isTrackRendering (t);
        if (! busy)
            break;
    }
    pump (60);   // let the timer run once more (it may re-request on parameter changes)
    for (int i = 0; i < 400; ++i)
    {
        bool busy = false;
        for (int t = 0; t < kNumTracks; ++t)
            busy = busy || p.isTrackRendering (t);
        if (! busy)
            break;
        pump (10);
    }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    const juce::File work (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    const auto samples = work.getChildFile ("samples");
    samples.deleteRecursively();

    const double fileRate = 44100.0;
    juce::Random rng (7);
    writeWav (samples.getChildFile ("Kicks/Kick Deep.wav"), kick (fileRate), fileRate);
    writeWav (samples.getChildFile ("Kicks/BD_02.wav"), kick (fileRate), fileRate);
    writeWav (samples.getChildFile ("Snares/Snare Crack.wav"), noise (fileRate, 0.3, 14.0, 0.35f, rng), fileRate);
    writeWav (samples.getChildFile ("Hats/HH Closed.wav"), noise (fileRate, 0.1, 45.0, 0.95f, rng), fileRate);
    writeWav (samples.getChildFile ("Misc/mystery hit.wav"), kick (fileRate), fileRate);   // no name hint -> by sound
    writeWav (samples.getChildFile ("Loops/long loop.wav"), noise (fileRate, 12.0, 0.0, 0.5f, rng), fileRate); // > 10 s, skipped

    {
        auto p = std::make_unique<StretchKitProcessor>();

        std::cout << "Library scan\n";
        p->library->addFolder (samples);
        for (int i = 0; i < 500; ++i)
        {
            pump (20);
            if (! p->library->isScanning() && p->library->getSnapshot()->size() >= 5)
                break;
        }
        const auto snap = p->library->getSnapshot();
        check (snap->size() == 5, "5 one-shots found, loop skipped (got " + juce::String ((int) snap->size()) + ")");
        for (const auto& e : *snap)
        {
            const bool expectKick = e.name.containsIgnoreCase ("kick") || e.name == "BD_02" || e.name == "mystery hit";
            const auto expected = expectKick ? Category::Kick
                                : e.name.containsIgnoreCase ("snare") ? Category::Snare : Category::HiHat;
            check (e.category == expected && e.x > 0.0f && e.x < 1.0f && e.y > 0.0f && e.y < 1.0f,
                   e.name + " -> " + categoryName (e.category) + " at (" + juce::String (e.x, 2) + ", " + juce::String (e.y, 2) + ")");
        }

        juce::Random r2 (3);
        const auto deep = samples.getChildFile ("Kicks/Kick Deep.wav").getFullPathName();
        const auto similar = p->library->findSimilar (deep, r2);
        check (similar.isNotEmpty() && p->library->categoryFor (similar) == Category::Kick,
               "similar to Kick Deep -> " + juce::File (similar).getFileName());

        std::cout << "Sequencer timing (internal clock, 120 BPM, 1/16, 48 kHz)\n";
        const double sr = 48000.0;
        p->setPlayConfigDetails (0, 2, sr, 512);
        p->prepareToPlay (sr, 512);
        pump (100);   // a fresh instance auto-loads a random kit from the library; override it:
        for (int t = 0; t < kNumTracks; ++t)
            p->setTrackSample (t, {});
        p->setTrackSample (0, deep);
        p->setTrackSample (1, samples.getChildFile ("Snares/Snare Crack.wav").getFullPathName());
        p->setTrackSample (3, samples.getChildFile ("Hats/HH Closed.wav").getFullPathName());
        waitForRenders (*p);

        std::array<int, kNumTracks> before {};
        for (int t = 0; t < kNumTracks; ++t) before[(size_t) t] = p->getHitCounter (t);

        p->setInternalPlaying (true);
        const int bar = (int) (sr * 2.0);              // one 4/4 bar at 120 BPM
        auto out = render (*p, bar - 100, 512);
        p->setInternalPlaying (false);

        const int kickHits = p->getHitCounter (0) - before[0];
        const int snareHits = p->getHitCounter (1) - before[1];
        const int hatHits = p->getHitCounter (3) - before[3];
        check (kickHits == 5 && snareHits == 2 && hatHits == 8,
               "hits in one bar: kick " + juce::String (kickHits) + " (5), snare " + juce::String (snareHits)
               + " (2), hat " + juce::String (hatHits) + " (8)");

        // sample accuracy: only the kick, only on step 5 -> must start at 5 * 6000 samples
        render (*p, 48000, 512);   // let tails die out
        juce::String savedPattern;
        for (int t = 0; t < kNumTracks; ++t)
            for (int s = 0; s < kMaxSteps; ++s)
                savedPattern << p->getStep (t, s);
        for (int t = 0; t < kNumTracks; ++t)
            p->clearTrackSteps (t);
        p->setStep (0, 5, 2);
        p->setInternalPlaying (true);
        auto solo = render (*p, bar, 333);   // odd block size on purpose
        p->setInternalPlaying (false);
        int onset = -1;
        for (int i = 0; i < solo.getNumSamples(); ++i)
            if (std::abs (solo.getSample (0, i)) > 0.0f) { onset = i; break; }
        check (onset == 30000, "step 6 hit starts at sample " + juce::String (onset) + " (30000)");
        for (int t = 0; t < kNumTracks; ++t)
            for (int s = 0; s < kMaxSteps; ++s)
                p->setStep (t, s, savedPattern[t * kMaxSteps + s] - '0');

        std::cout << "Length % on a single MIDI hit\n";
        render (*p, 48000, 512);   // let tails die out
        for (auto t : { 1, 3 }) setParam (*p, "mute" + juce::String (t), 1.0f);

        juce::MidiBuffer hit;
        hit.addEvent (juce::MidiMessage::noteOn (1, kFirstNote, (juce::uint8) 127), 0);

        int len100 = 0;
        for (auto pct : { 100.0f, 200.0f, 50.0f })
        {
            setParam (*p, "len0", pct);
            pump (120);
            waitForRenders (*p);
            render (*p, 512, 512);      // swap in the new render
            auto o = render (*p, (int) (sr * 1.5), 512, &hit);
            const int len = audibleLength (o);
            if (pct == 100.0f) len100 = len;
            const double ratio = len100 > 0 ? (double) len / len100 : 0.0;
            check (std::abs (ratio - pct / 100.0) < 0.12,
                   juce::String ((int) pct) + "%: audible " + juce::String (len) + " samples (ratio " + juce::String (ratio, 2) + ")");
            writeWav (work.getChildFile ("out_kick_" + juce::String ((int) pct) + ".wav"), o, sr);
        }

        std::cout << "State save / restore\n";
        p->setStep (5, 7, 2);
        juce::MemoryBlock state;
        p->getStateInformation (state);

        auto q = std::make_unique<StretchKitProcessor>();
        q->setStateInformation (state.getData(), (int) state.getSize());
        check (q->getTrackPath (0) == deep && q->getStep (5, 7) == 2 && q->getStep (0, 0) == 2
               && std::abs (q->apvts.getRawParameterValue ("len0")->load() - 50.0f) < 0.01f,
               "paths, pattern and length restored");

        std::cout << "MIDI export\n";
        const auto midiFile = p->exportPatternAsMidi();
        juce::MidiFile mf;
        juce::FileInputStream in (midiFile);
        const bool read = in.openedOk() && mf.readFrom (in);
        int notes = 0;
        if (read)
            for (auto* ev : *mf.getTrack (0))
                if (ev->message.isNoteOn()) ++notes;
        check (read && notes == 16, "pattern .mid has " + juce::String (notes) + " notes (16)");

        std::cout << "Editor\n";
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor (p->createEditor());
            editor->setVisible (true);
            pump (400);
            auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            auto f = work.getChildFile ("editor.png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            png.writeImageToStream (img, os);
            check (img.getWidth() == 1120 && img.getHeight() == 800, "editor snapshot written to editor.png");
        }

        q.reset();
        p.reset();
    }

    std::cout << (failures == 0 ? "\nALL PASSED\n" : "\nFAILURES: " + std::to_string (failures) + "\n");
    return failures == 0 ? 0 : 1;
}
