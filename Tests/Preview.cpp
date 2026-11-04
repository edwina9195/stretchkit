// Renders a screenshot of every skin with a realistic demo state:
// ~170 generated one-shots on the map, 8 loaded tracks, a pattern playing.
// Build with -DSTRETCHKIT_BUILD_HOST_TEST=ON, run: StretchKitPreview <work-folder>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <iostream>

using namespace sk;

namespace
{
constexpr double fsr = 44100.0;
constexpr double twoPi = juce::MathConstants<double>::twoPi;

void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& b)
{
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (file.createOutputStream().release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (fsr)
                                                                          .withNumChannels (1)
                                                                          .withBitsPerSample (16));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

struct Synth
{
    juce::Random rng { 99 };

    float noise() { return rng.nextFloat() * 2.0f - 1.0f; }
    float uni (float a, float b) { return a + (b - a) * rng.nextFloat(); }

    juce::AudioBuffer<float> make (double seconds, const std::function<float (double, int)>& fn)
    {
        juce::AudioBuffer<float> b (1, (int) (seconds * fsr));
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (0, i, fn (i / fsr, i));
        b.applyGain (0.9f / juce::jmax (0.001f, b.getMagnitude (0, b.getNumSamples())));
        return b;
    }

    juce::AudioBuffer<float> kick()
    {
        const float f0 = uni (90, 180), f1 = uni (38, 60), pd = uni (18, 45), ad = uni (4, 12), click = uni (0, 0.5f);
        double ph = 0.0;
        return make (uni (0.35f, 0.8f), [=, &ph] (double t, int) mutable
        {
            ph += twoPi * (f1 + (f0 - f1) * std::exp (-t * pd)) / fsr;
            return (float) (std::sin (ph) * std::exp (-t * ad)) + click * noise() * (float) std::exp (-t * 400.0);
        });
    }

    juce::AudioBuffer<float> snare()
    {
        const float tone = uni (160, 260), nd = uni (8, 22), lpA = uni (0.15f, 0.5f), mix = uni (0.3f, 0.8f);
        float lp = 0, prev = 0, hp = 0;
        return make (uni (0.2f, 0.45f), [=, &lp, &prev, &hp] (double t, int) mutable
        {
            lp += lpA * (noise() - lp); hp = 0.9f * (hp + lp - prev); prev = lp;
            return mix * hp * (float) std::exp (-t * nd) + (1.0f - mix) * (float) (std::sin (twoPi * tone * t) * std::exp (-t * 30.0));
        });
    }

    juce::AudioBuffer<float> clap()
    {
        const float gap = uni (0.008f, 0.014f), tail = uni (10, 25);
        float lp = 0, prev = 0, hp = 0;
        return make (uni (0.25f, 0.5f), [=, &lp, &prev, &hp] (double t, int) mutable
        {
            lp += 0.25f * (noise() - lp); hp = 0.9f * (hp + lp - prev); prev = lp;
            double env = 0.0;
            for (int k = 0; k < 3; ++k)
                if (t >= k * gap && t < (k + 1) * gap) env = std::exp (-(t - k * gap) * 300.0);
            if (t >= 3 * gap) env = std::exp (-(t - 3 * gap) * tail);
            return hp * (float) env;
        });
    }

    juce::AudioBuffer<float> hat (bool open)
    {
        const float d = open ? uni (5, 9) : uni (25, 70), lpA = uni (0.7f, 0.98f);
        float lp = 0, prev = 0, hp = 0;
        return make (open ? 0.6 : 0.15, [=, &lp, &prev, &hp] (double t, int) mutable
        {
            lp += lpA * (noise() - lp); hp = 0.5f * (hp + lp - prev); prev = lp;
            return hp * (float) std::exp (-t * d);
        });
    }

    juce::AudioBuffer<float> cymbal()
    {
        const float d = uni (1.0f, 2.2f);
        float prev = 0, hp = 0;
        return make (uni (1.8f, 3.0f), [=, &prev, &hp] (double t, int) mutable
        {
            const float x = noise();
            hp = 0.45f * (hp + x - prev); prev = x;
            return hp * (float) std::exp (-t * d);
        });
    }

    juce::AudioBuffer<float> tom()
    {
        const float f = uni (80, 260), d = uni (4, 9);
        double ph = 0.0;
        return make (uni (0.4f, 0.8f), [=, &ph] (double t, int) mutable
        {
            ph += twoPi * (f * (1.0 + 0.4 * std::exp (-t * 25.0))) / fsr;
            return (float) (std::sin (ph) * std::exp (-t * d));
        });
    }

    juce::AudioBuffer<float> perc (int kind)
    {
        switch (kind % 4)
        {
            case 0: { const float f = uni (1200, 2200);   // rim
                      return make (0.08, [=] (double t, int) { return (float) (std::sin (twoPi * f * t) * std::exp (-t * 60.0)) + 0.3f * noise() * (float) std::exp (-t * 300.0); }); }
            case 1: { const float a = uni (520, 580), b = uni (780, 840);   // cowbell
                      return make (0.3, [=] (double t, int) { return (float) ((std::sin (twoPi * a * t) > 0 ? 1 : -1) + (std::sin (twoPi * b * t) > 0 ? 1 : -1)) * 0.5f * (float) std::exp (-t * 12.0); }); }
            case 2: { float lp = 0;   // shaker
                      return make (0.18, [=, &lp] (double t, int) mutable { lp += 0.6f * (noise() - lp); return (noise() - lp) * (float) (std::min (1.0, t / 0.015) * std::exp (-t * 22.0)); }); }
            default: { const float f = uni (250, 420); double ph = 0.0;   // conga
                       return make (0.35, [=, &ph] (double t, int) mutable { ph += twoPi * f * (1.0 + 0.15 * std::exp (-t * 40.0)) / fsr; return (float) (std::sin (ph) * std::exp (-t * 12.0)); }); }
        }
    }

    juce::AudioBuffer<float> fx()
    {
        const float f0 = uni (1500, 4000), f1 = uni (60, 200), sweep = uni (8, 30);
        double ph = 0.0;
        return make (uni (0.3f, 0.9f), [=, &ph] (double t, int) mutable
        {
            ph += twoPi * (f1 + (f0 - f1) * std::exp (-t * sweep)) / fsr;
            return (float) (std::sin (ph) * std::exp (-t * 4.0));
        });
    }
};

juce::String num (int i) { return juce::String (i).paddedLeft ('0', 2); }
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File work (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    const auto kit = work.getChildFile ("Demo Kit");
    kit.deleteRecursively();

    Synth s;
    const char* kickAdj[]  = { "Punch", "Deep", "Sub", "Tight", "Boom", "Dusty", "Hard", "Soft", "Knock", "Thump" };
    const char* snareAdj[] = { "Crack", "Fat", "Dry", "Snap", "Lofi", "Tight", "Wide", "Rattle" };
    const char* tomAdj[]   = { "Low", "Mid", "High", "Floor" };
    const char* percName[] = { "Rim Tick", "Cowbell", "Shaker", "Conga" };
    const char* fxName[]   = { "Zap", "Laser", "Blip", "Drop", "Ping" };

    for (int i = 0; i < 30; ++i) writeWav (kit.getChildFile ("Kicks/Kick " + juce::String (kickAdj[i % 10]) + " " + num (i / 10 + 1) + ".wav"), s.kick());
    for (int i = 0; i < 24; ++i) writeWav (kit.getChildFile ("Snares/Snare " + juce::String (snareAdj[i % 8]) + " " + num (i / 8 + 1) + ".wav"), s.snare());
    for (int i = 0; i < 16; ++i) writeWav (kit.getChildFile ("Claps/Clap " + num (i + 1) + ".wav"), s.clap());
    for (int i = 0; i < 20; ++i) writeWav (kit.getChildFile ("Hats/HH Closed " + num (i + 1) + ".wav"), s.hat (false));
    for (int i = 0; i < 10; ++i) writeWav (kit.getChildFile ("Hats/HH Open " + num (i + 1) + ".wav"), s.hat (true));
    for (int i = 0; i < 12; ++i) writeWav (kit.getChildFile (juce::String (i % 2 == 0 ? "Cymbals/Crash " : "Cymbals/Ride ") + num (i / 2 + 1) + ".wav"), s.cymbal());
    for (int i = 0; i < 16; ++i) writeWav (kit.getChildFile ("Toms/Tom " + juce::String (tomAdj[i % 4]) + " " + num (i / 4 + 1) + ".wav"), s.tom());
    for (int i = 0; i < 28; ++i) writeWav (kit.getChildFile ("Perc/" + juce::String (percName[i % 4]) + " " + num (i / 4 + 1) + ".wav"), s.perc (i));
    for (int i = 0; i < 10; ++i) writeWav (kit.getChildFile ("FX/" + juce::String (fxName[i % 5]) + " " + num (i / 5 + 1) + ".wav"), s.fx());

    auto p = std::make_unique<StretchKitProcessor>();
    p->library->addFolder (kit);
    for (int i = 0; i < 1000; ++i)
    {
        pump (20);
        if (! p->library->isScanning() && p->library->getSnapshot()->size() >= 160)
            break;
    }
    std::cout << "samples on map: " << p->library->getSnapshot()->size() << "\n";

    const double sr = 48000.0;
    p->setPlayConfigDetails (0, 2, sr, 512);
    p->prepareToPlay (sr, 512);
    pump (100);

    const juce::String tracks[kNumTracks] = { "Kicks/Kick Punch 01.wav", "Snares/Snare Crack 01.wav", "Claps/Clap 03.wav",
                                              "Hats/HH Closed 04.wav", "Hats/HH Open 02.wav", "Toms/Tom Low 02.wav",
                                              "Perc/Rim Tick 01.wav", "Cymbals/Crash 01.wav" };
    for (int t = 0; t < kNumTracks; ++t)
        p->setTrackSample (t, kit.getChildFile (tracks[t]).getFullPathName());

    for (int t = 0; t < kNumTracks; ++t)
        p->clearTrackSteps (t);
    auto steps = [&] (int t, std::initializer_list<int> on, std::initializer_list<int> accents)
    {
        for (int st : on) p->setStep (t, st, 1);
        for (int st : accents) p->setStep (t, st, 2);
    };
    steps (0, { 7, 10 }, { 0, 6 });
    steps (1, {}, { 4, 12 });
    steps (2, { 12 }, {});
    steps (3, { 0, 2, 3, 6, 8, 10, 11 }, { 4, 12 });
    steps (4, {}, { 14 });
    steps (5, { 13 }, { 15 });
    steps (6, { 3, 9, 11 }, {});
    steps (7, {}, { 0 });

    auto set = [&] (const juce::String& id, float v)
    {
        auto* param = p->apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (v));
    };
    set ("len1", 160.0f); set ("len2", 120.0f); set ("len3", 70.0f); set ("len7", 250.0f);
    set ("pitch5", -3.0f); set ("pan3", 0.25f); set ("pan6", -0.3f); set ("vol7", -6.0f);
    set ("swing", 12.0f);

    for (int i = 0; i < 300; ++i)
    {
        pump (10);
        bool busy = false;
        for (int t = 0; t < kNumTracks; ++t) busy = busy || p->isTrackRendering (t);
        if (! busy && i > 20) break;
    }

    // play up to step 6 so the playhead shows
    p->setInternalPlaying (true);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < 36100; pos += 512)
        p->processBlock (buf, midi);

    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    auto* editor = dynamic_cast<StretchKitEditor*> (ed.get());
    editor->setVisible (true);

    const char* fileNames[kNumThemes] = { "skin_A_ps2_night", "skin_B_y2k_chrome", "skin_C_digicore", "skin_D_mix", "skin_E_liquid_chrome" };
    for (int i = 0; i < kNumThemes; ++i)
    {
        editor->applyTheme ((ThemeId) i);
        pump (250);
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto f = work.getChildFile (juce::String (fileNames[i]) + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "wrote " << f.getFullPathName() << "\n";
    }

    ed.reset();
    p.reset();
    return 0;
}
