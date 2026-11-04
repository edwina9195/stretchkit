#pragma once

#include "library/SampleLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace sk
{
/** XO-style strip under the map: the selected sound (seed) and its nearest neighbours.

    |<  <  >    step through the similar sounds (> plays them one after another)
    dots        click to hear, drag onto a track to load
    History     up / down goes back through the sounds you picked before
    Heart       mark the current sound as a favourite
    Load        put the current sound on the selected track */
class SimilarityBar : public juce::Component,
                      public juce::SettableTooltipClient,
                      private juce::Timer
{
public:
    static constexpr int kNeighbours = 24;

    SimilarityBar();

    /** Makes `index` (into the current snapshot) the seed and finds its neighbours. */
    void setSeed (LibrarySnapshot snap, int index, bool addToHistory = true);
    void clear();

    int getSeed() const noexcept { return seed; }
    int getCurrent() const noexcept { return current >= 0 ? current : seed; }
    const std::vector<int>& getNeighbours() const noexcept { return neighbours; }

    std::function<void (int index)> onAudition;               // play this sound
    std::function<void (int index)> onSeedChanged;            // new seed (from history)
    std::function<void()> onSelectionChanged;                 // anything the map should redraw
    std::function<bool (const juce::String&)> isFavourite;
    std::function<void (const juce::String&, bool)> setFavourite;
    std::function<void (const juce::String&)> onLoad;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    enum class Part { none, first, prev, play, dot, history, histUp, histDown, heart, load };
    struct Hit { Part part = Part::none; int slot = -1; };

    Hit hitAt (juce::Point<float>) const;
    void layout (juce::Rectangle<float>& first, juce::Rectangle<float>& prev, juce::Rectangle<float>& play,
                 juce::Rectangle<float>& dots, juce::Rectangle<float>& hist, juce::Rectangle<float>& up,
                 juce::Rectangle<float>& down, juce::Rectangle<float>& heart, juce::Rectangle<float>& load) const;
    juce::Point<float> slotCentre (int slot) const;       // slot 0 = seed, 1.. = neighbours
    int indexForSlot (int slot) const;
    void playSlot (int slot);
    void step (int delta);
    void goHistory (int delta);
    void timerCallback() override;

    LibrarySnapshot snapshot;
    int seed = -1, current = -1, currentSlot = 0;
    std::vector<int> neighbours;
    std::vector<juce::String> history;      // paths, newest last
    int historyPos = -1;
    bool playingThrough = false;
    Hit hover, pressed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimilarityBar)
};
} // namespace sk
