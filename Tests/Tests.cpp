// Offline checks for the stretch engine and the sample classifier.
// Build with -DSTRETCHKIT_BUILD_TESTS=ON, run: StretchKitTests [output-folder-for-wavs]

#include "dsp/DrumStretcher.h"
#include "library/FeatureExtractor.h"

#include <cmath>
#include <iostream>

using namespace sk;

namespace
{
constexpr double sr = 44100.0;
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << "\n";
    if (! ok) ++failures;
}

juce::AudioBuffer<float> makeBuffer (double seconds)
{
    juce::AudioBuffer<float> b (2, (int) (seconds * sr));
    b.clear();
    return b;
}

void setBoth (juce::AudioBuffer<float>& b, int i, float v) { b.setSample (0, i, v); b.setSample (1, i, v); }

juce::AudioBuffer<float> synthKick()
{
    auto b = makeBuffer (0.6);
    double phase = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr;
        const double f = 45.0 + 110.0 * std::exp (-t * 30.0);
        phase += juce::MathConstants<double>::twoPi * f / sr;
        setBoth (b, i, (float) (std::sin (phase) * std::exp (-t * 7.0) * 0.9));
    }
    return b;
}

juce::AudioBuffer<float> noiseHit (double seconds, double decay, double hpCoeff, juce::Random& r, double toneHz = 0.0, double lpHz = 0.0)
{
    auto b = makeBuffer (seconds);
    float prevIn = 0.0f, prevOut = 0.0f, lp = 0.0f;
    const float lpA = lpHz > 0.0 ? (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * lpHz / sr)) : 1.0f;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr;
        const float x = r.nextFloat() * 2.0f - 1.0f;
        const float hp = (float) hpCoeff * (prevOut + x - prevIn);   // one-pole high-pass
        prevIn = x; prevOut = hp;
        lp += lpA * (hp - lp);
        float s = lp * (float) std::exp (-t * decay);
        if (toneHz > 0.0)
            s = 0.6f * s + 0.5f * (float) (std::sin (juce::MathConstants<double>::twoPi * toneHz * t) * std::exp (-t * 25.0));
        setBoth (b, i, s * 0.8f);
    }
    return b;
}

juce::AudioBuffer<float> synthClap (juce::Random& r)
{
    auto b = makeBuffer (0.4);
    const double starts[] = { 0.0, 0.011, 0.022, 0.033 };
    float lp1 = 0.0f, hp1 = 0.0f, prev = 0.0f;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr;
        double env = 0.0;
        for (int k = 0; k < 3; ++k)
            if (t >= starts[k] && t < starts[k + 1]) env = std::exp (-(t - starts[k]) * 300.0);
        if (t >= starts[3]) env = std::exp (-(t - starts[3]) * 18.0);
        // band-ish noise around 1-2 kHz
        const float x = (r.nextFloat() * 2.0f - 1.0f);
        lp1 += 0.25f * (x - lp1);           // ~1.9 kHz low-pass
        hp1 = 0.9f * (hp1 + lp1 - prev);    // high-pass
        prev = lp1;
        setBoth (b, i, (float) (hp1 * env * 2.0));
    }
    return b;
}

juce::AudioBuffer<float> synthTom()
{
    auto b = makeBuffer (0.7);
    double phase = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr;
        const double f = 160.0 + 60.0 * std::exp (-t * 20.0);
        phase += juce::MathConstants<double>::twoPi * f / sr;
        setBoth (b, i, (float) (std::sin (phase) * std::exp (-t * 6.0) * 0.8));
    }
    return b;
}

void writeWav (const juce::File& dir, const juce::String& name, const juce::AudioBuffer<float>& b, double rate)
{
    if (dir == juce::File())
        return;
    dir.createDirectory();
    auto file = dir.getChildFile (name + ".wav");
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (file.createOutputStream().release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (rate)
                                                                          .withNumChannels (b.getNumChannels())
                                                                          .withBitsPerSample (24));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int firstAbove (const juce::AudioBuffer<float>& b, float thresh)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
        if (std::abs (b.getSample (0, i)) > thresh) return i;
    return -1;
}
} // namespace

int main (int argc, char** argv)
{
    const juce::File outDir = argc > 1 ? juce::File (juce::String (argv[1])) : juce::File();
    juce::Random rng (1234);

    struct Named { const char* name; juce::AudioBuffer<float> buf; Category expected; };
    std::vector<Named> sounds;
    sounds.push_back ({ "kick",   synthKick(),                                 Category::Kick });
    sounds.push_back ({ "snare",  noiseHit (0.35, 14.0, 0.9, rng, 190.0, 3000.0),     Category::Snare });
    sounds.push_back ({ "hat",    noiseHit (0.12, 45.0, 0.25, rng),            Category::HiHat });
    sounds.push_back ({ "cymbal", noiseHit (2.5, 1.5, 0.25, rng),              Category::Cymbal });
    sounds.push_back ({ "clap",   synthClap (rng),                             Category::Clap });
    sounds.push_back ({ "tom",    synthTom(),                                  Category::Tom });

    std::cout << "Classification by sound (neutral file names)\n";
    for (auto& s : sounds)
    {
        auto f = FeatureExtractor::analyse (s.buf, sr, (float) (s.buf.getNumSamples() / sr));
        auto c = FeatureExtractor::classifyBySound (f);
        check (c == s.expected, juce::String (s.name) + " -> " + categoryName (c)
                                 + "  (centroid " + juce::String ((int) f.centroidHz) + " Hz, low "
                                 + juce::String (f.v[fLow], 2) + ", flat " + juce::String (f.v[fFlatness], 3)
                                 + ", decay " + juce::String (f.decaySec, 2) + " s, bursts " + juce::String (f.bursts) + ")");
        writeWav (outDir, juce::String ("src_") + s.name, s.buf, sr);
    }

    std::cout << "Classification by name\n";
    check (FeatureExtractor::classifyByName (juce::File ("/x/Pack/BD_Tight01.wav")) == Category::Kick,  "BD_Tight01 -> Kick");
    check (FeatureExtractor::classifyByName (juce::File ("/x/Pack/OpenHat 3.wav"))  == Category::HiHat, "OpenHat 3 -> Hat");
    check (FeatureExtractor::classifyByName (juce::File ("/x/Snares/hit 4.wav"))    == Category::Snare, "Snares/hit 4 -> Snare");
    check (FeatureExtractor::classifyByName (juce::File ("/x/Pack/CP_909.wav"))     == Category::Clap,  "CP_909 -> Clap");
    check (! FeatureExtractor::classifyByName (juce::File ("/x/Pack/thing 4.wav")).has_value(),         "thing 4 -> no name match");

    std::cout << "Resampler alignment\n";
    {
        juce::AudioBuffer<float> imp (1, 2000);
        imp.clear();
        imp.setSample (0, 500, 1.0f);
        auto slow = DrumStretcher::resample (imp, 0.5);
        int peak = 0;
        for (int i = 0; i < slow.getNumSamples(); ++i)
            if (std::abs (slow.getSample (0, i)) > std::abs (slow.getSample (0, peak))) peak = i;
        check (std::abs (peak - 1000) <= 2, "impulse at 500 -> " + juce::String (peak) + " at half speed (expect 1000)");
        check (std::abs (slow.getNumSamples() - 4000) <= 1, "length 2000 -> " + juce::String (slow.getNumSamples()));
    }

    std::cout << "Rendering (44.1k source -> 48k host)\n";
    const double target = 48000.0;
    const float percents[] = { 50.0f, 100.0f, 200.0f, 400.0f };
    const StretchMode modes[] = { StretchMode::Hybrid, StretchMode::Smooth, StretchMode::Tape };
    const char* modeNames[] = { "hybrid", "smooth", "tape" };

    for (auto& s : sounds)
    {
        const int srcLen = s.buf.getNumSamples();
        for (int mi = 0; mi < 3; ++mi)
        {
            for (auto pct : percents)
            {
                RenderSettings rs;
                rs.sourceRate = sr; rs.targetRate = target; rs.lengthPercent = pct; rs.mode = modes[mi];
                const auto t0 = juce::Time::getMillisecondCounterHiRes();
                auto out = DrumStretcher::render (s.buf, rs);
                const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
                const int expect = (int) std::lround (srcLen / (sr / target) * pct / 100.0);

                bool finite = true;
                float peak = 0.0f;
                for (int c = 0; c < out.getNumChannels(); ++c)
                    for (int i = 0; i < out.getNumSamples(); ++i)
                    {
                        const float v = out.getSample (c, i);
                        finite = finite && std::isfinite (v);
                        peak = juce::jmax (peak, std::abs (v));
                    }

                const bool onsetOk = std::abs (firstAbove (out, 0.05f) - (int) (firstAbove (s.buf, 0.05f) * target / sr)) <= 3
                                     || modes[mi] != StretchMode::Hybrid;

                check (out.getNumSamples() == expect && finite && peak > 0.05f && peak < 2.0f && onsetOk,
                       juce::String (s.name) + " " + modeNames[mi] + " " + juce::String ((int) pct) + "%: "
                       + juce::String (out.getNumSamples()) + " samples (expect " + juce::String (expect) + "), peak "
                       + juce::String (peak, 2) + ", " + juce::String (ms, 1) + " ms");

                if (pct == 200.0f || pct == 50.0f)
                    writeWav (outDir, juce::String (s.name) + "_" + modeNames[mi] + "_" + juce::String ((int) pct), out, target);
            }
        }
    }

    std::cout << "Hybrid keeps the attack sample-for-sample\n";
    {
        RenderSettings rs;
        rs.sourceRate = sr; rs.targetRate = sr; rs.lengthPercent = 250.0f; rs.mode = StretchMode::Hybrid;
        auto& snare = sounds[1].buf;
        auto out = DrumStretcher::render (snare, rs);
        const int attackEnd = DrumStretcher::findAttackEnd (snare, sr);
        const int keep = attackEnd - (int) (0.004 * sr);
        float maxDiff = 0.0f;
        for (int i = 0; i < keep; ++i)
            maxDiff = juce::jmax (maxDiff, std::abs (out.getSample (0, i) - snare.getSample (0, i)));
        check (keep > 100 && maxDiff < 1.0e-6f, "first " + juce::String (keep) + " samples identical (max diff "
                                                  + juce::String (maxDiff) + ")");
    }

    std::cout << "Pitch shift keeps the requested length\n";
    {
        RenderSettings rs;
        rs.sourceRate = sr; rs.targetRate = sr; rs.lengthPercent = 100.0f; rs.pitchSemitones = 7.0f;
        auto out = DrumStretcher::render (sounds[0].buf, rs);
        check (out.getNumSamples() == sounds[0].buf.getNumSamples(), "kick +7 st at 100%: "
               + juce::String (out.getNumSamples()) + " samples");
        writeWav (outDir, "kick_pitch+7_100", out, sr);
    }

    std::cout << (failures == 0 ? "\nALL PASSED\n" : "\nFAILURES: " + std::to_string (failures) + "\n");
    return failures == 0 ? 0 : 1;
}
