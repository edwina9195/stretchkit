// Large-library check: scan speed, a scan interrupted by closing the plugin, automatic resume,
// instant reload, and map drawing time. Renders screenshots of the map with the similarity bar.
// usage: StretchKitStress <work-folder> [numFiles]

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <iostream>

using namespace sk;

namespace
{
constexpr double fsr = 16000.0;
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << "\n" << std::flush;
    if (! ok) ++failures;
}

void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& b)
{
    file.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (file.createOutputStream().release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (fsr).withNumChannels (1).withBitsPerSample (16));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

juce::AudioBuffer<float> synth (int kind, juce::Random& rng)
{
    const double twoPi = juce::MathConstants<double>::twoPi;
    auto uni = [&] (float a, float b) { return a + (b - a) * rng.nextFloat(); };
    const double len = kind == 4 ? 0.45 : 0.12 + 0.18 * rng.nextFloat();
    juce::AudioBuffer<float> b (1, (int) (len * fsr));
    double ph = 0.0;
    float lp = 0.0f, prev = 0.0f, hp = 0.0f;
    const float f0 = uni (60, 260), d = uni (6, 40), lpA = uni (0.1f, 0.95f), mix = uni (0.0f, 1.0f);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / fsr;
        const float noise = rng.nextFloat() * 2.0f - 1.0f;
        lp += lpA * (noise - lp); hp = 0.8f * (hp + lp - prev); prev = lp;
        float s = 0.0f;
        switch (kind)
        {
            case 0: ph += twoPi * (f0 * 0.5 + f0 * 1.5 * std::exp (-t * 30.0)) / fsr; s = (float) (std::sin (ph) * std::exp (-t * d * 0.4)); break; // kick
            case 1: ph += twoPi * f0 * 1.2 / fsr; s = mix * hp * (float) std::exp (-t * d * 0.6) + (1 - mix) * (float) (std::sin (ph) * std::exp (-t * 30.0)); break; // snare
            case 2: s = hp * (float) std::exp (-t * (40.0 + d)); break;                           // hat
            case 3: ph += twoPi * f0 * 0.9 / fsr; s = (float) (std::sin (ph) * std::exp (-t * d * 0.25)); break; // tom
            case 4: s = hp * (float) std::exp (-t * 3.0); break;                                  // cymbal
            default: ph += twoPi * f0 * 6.0 / fsr; s = (float) (std::sin (ph) * std::exp (-t * 50.0)); break; // perc
        }
        b.setSample (0, i, s);
    }
    b.applyGain (0.9f / juce::jmax (0.001f, b.getMagnitude (0, b.getNumSamples())));
    return b;
}

void waitForScan (StretchKitProcessor& p, int timeoutSec)
{
    const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutSec * 1000;
    while (juce::Time::getMillisecondCounter() < end)
    {
        pump (100);
        if (p.library->isLoaded() && ! p.library->isScanning())
        {
            pump (500);   // a resumed scan may only just be starting
            if (! p.library->isScanning())
                return;
        }
    }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File work { juce::String (argv[1]) };
    const int numFiles = argc > 2 ? juce::String (argv[2]).getIntValue() : 12000;
    const auto lib = work.getChildFile ("Big Library");

    if (lib.findChildFiles (juce::File::findFiles, true, "*.wav").size() != numFiles)
    {
        lib.deleteRecursively();
        juce::Random rng (5);
        const char* folders[] = { "Kicks", "Snares", "Hats", "Toms", "Cymbals", "Perc" };
        for (int i = 0; i < numFiles; ++i)
        {
            const int kind = i % 6;
            // half the files have no category hint in their name, so they are sorted by sound
            const auto name = (i % 2 == 0 ? juce::String (folders[kind]) + "/" + juce::String (folders[kind]).dropLastCharacters (1) + " "
                                          : juce::String ("Unsorted/hit ")) + juce::String (i).paddedLeft ('0', 6) + ".wav";
            writeWav (lib.getChildFile (name), synth (kind, rng));
        }
        std::cout << "generated " << numFiles << " files\n";
    }

    std::cout << "First scan, interrupted by closing the plugin\n";
    double fullRate = 0.0;
    {
        auto p = std::make_unique<StretchKitProcessor>();
        while (! p->library->isLoaded()) pump (10);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        p->library->addFolder (lib);
        while (p->library->getScannedCount() < numFiles * 45 / 100) pump (50);
        const double secs = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
        fullRate = p->library->getScannedCount() / secs;
        std::cout << "  scanned " << p->library->getScannedCount() << " in " << secs << " s ("
                  << (int) fullRate << " files/s on " << juce::SystemStats::getNumCpus() << " CPUs)\n";
        p.reset();   // closes the plugin mid-scan
    }

    std::cout << "Reopen: the scan continues by itself\n";
    {
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        auto p = std::make_unique<StretchKitProcessor>();
        while (! p->library->isLoaded()) pump (10);
        const int restored = (int) p->library->getSnapshot()->size();
        check (restored >= numFiles * 35 / 100, "progress kept from the interrupted scan: " + juce::String (restored) + " sounds on the map at once");
        waitForScan (*p, 600);
        const double secs = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
        const int total = (int) p->library->getSnapshot()->size();
        check (total == numFiles, "resumed scan finished: " + juce::String (total) + " / " + juce::String (numFiles)
                                  + " in " + juce::String (secs, 1) + " s");
    }

    std::cout << "Reopen again: nothing to scan\n";
    {
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        auto p = std::make_unique<StretchKitProcessor>();
        while (! p->library->isLoaded()) pump (5);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        pump (600);
        check ((int) p->library->getSnapshot()->size() == numFiles && ! p->library->isScanning(),
               "library loaded in " + juce::String ((int) ms) + " ms, no rescan");

        const auto snap = p->library->getSnapshot();
        std::array<int, kNumCategories> counts {};
        int byName = 0, bySoundRight = 0, bySoundTotal = 0;
        const Category expect[] = { Category::Kick, Category::Snare, Category::HiHat, Category::Tom, Category::Cymbal, Category::Perc };
        for (const auto& e : *snap)
        {
            ++counts[(size_t) e.category];
            const int i = e.name.getTrailingIntValue();
            if (e.path.contains ("Unsorted")) { ++bySoundTotal; bySoundRight += e.category == expect[i % 6] ? 1 : 0; }
            else ++byName;
        }
        juce::String c;
        for (int k = 0; k < kNumCategories; ++k) c << categoryName ((Category) k) << " " << counts[(size_t) k] << "  ";
        std::cout << "  " << c << "\n";
        std::cout << "  sorted by sound (no name hint): " << bySoundRight << " / " << bySoundTotal << " match the generator\n";

        // map drawing
        const double rate = 48000.0;
        p->setPlayConfigDetails (0, 2, rate, 512);
        p->prepareToPlay (rate, 512);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        auto* editor = dynamic_cast<StretchKitEditor*> (ed.get());
        editor->setVisible (true);
        pump (300);

        for (auto [theme, file] : { std::pair<int, const char*> { 3, "big_mix.png" }, { 4, "big_liquid.png" }, { 2, "big_digicore.png" } })
        {
            editor->applyTheme ((ThemeId) theme);
            editor->pickSample ((int) snap->size() / 3, false);
            pump (200);
            const auto t1 = juce::Time::getMillisecondCounterHiRes();
            auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            const double drawMs = juce::Time::getMillisecondCounterHiRes() - t1;
            auto f = work.getChildFile (file);
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (img, os);
            std::cout << "  " << file << " drawn in " << (int) drawMs << " ms (first draw after a change)\n";
        }

        // redraw without changes must be cheap (cached dots)
        const auto t2 = juce::Time::getMillisecondCounterHiRes();
        auto img2 = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const double cachedMs = juce::Time::getMillisecondCounterHiRes() - t2;
        check (cachedMs < 400.0, "cached redraw " + juce::String ((int) cachedMs) + " ms");
        ed.reset();
    }

    std::cout << (failures == 0 ? "\nALL PASSED\n" : "\nFAILURES: " + std::to_string (failures) + "\n");
    return failures == 0 ? 0 : 1;
}
