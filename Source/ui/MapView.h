#pragma once

#include "library/SampleLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace sk
{
/** The sample map, XO style: one cloud of dots, similar sounds close together.

    Click a dot             listen + make it the similarity seed
    Click and drag          scrub: every sound you pass over plays
    Drag out of the map     carries the last sound you heard onto a track
    Double-click a dot      load it on the selected track
    Wheel / pinch           zoom   (trackpad two-finger scroll moves around)
    Cmd-drag or right-drag  move around;  double-click empty space resets the view

    The dots are drawn into a cached image straight from pixels, so 100k+ samples stay smooth. */
class MapView : public juce::Component,
                public juce::SettableTooltipClient
{
public:
    MapView();

    void setSnapshot (LibrarySnapshot snap);
    LibrarySnapshot getSnapshot() const { return snapshot; }

    void setCategoryVisible (Category c, bool visible);
    bool isCategoryVisible (Category c) const { return visible[(size_t) c]; }

    /** Text search (all words must appear in the name) and the favourites-only switch. */
    void setFilter (const juce::String& query, bool favouritesOnly, std::function<bool (const juce::String&)> isFavourite);

    void setHighlightedPath (const juce::String& path);         // the selected track's sample
    void setSelection (int seedIndex, const std::vector<int>& neighbours, int playingIndex);
    void setStatusText (const juce::String& text);

    std::function<void (int index, bool fromScrub)> onPick;      // a dot was clicked or scrubbed over
    std::function<void (const juce::String& path)> onAssign;    // double-click

    void paint (juce::Graphics&) override;
    void resized() override { dirty = true; }
    void lookAndFeelChanged() override { dirty = true; }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;

private:
    juce::Rectangle<float> mapArea() const;
    float pixelsPerUnit() const;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> toWorld (juce::Point<float> p) const;
    void zoomAround (juce::Point<float> anchor, float factor);
    void clampView();
    void rebuildCache();
    int findDot (juce::Point<float> p, float radius) const;
    bool passesFilter (const SampleEntry&) const;
    float dotRadius() const;

    LibrarySnapshot snapshot;
    std::array<bool, kNumCategories> visible;
    juce::StringArray searchWords;
    bool favouritesOnly = false;
    std::function<bool (const juce::String&)> isFavourite;

    juce::String highlightedPath, statusText;
    int seed = -1, playing = -1, hovered = -1, lastScrub = -1;
    std::vector<int> neighbours;

    float zoom = 1.0f;
    juce::Point<float> centre { kWorldW * 0.5f, 0.5f }, panStartCentre;
    enum class Gesture { none, scrub, pan, dragOut } gesture = Gesture::none;
    juce::uint32 lastScrubTime = 0;

    // render cache
    bool dirty = true;
    juce::Image cache;
    juce::Rectangle<int> cacheBounds;
    int cacheTheme = -1, visibleCount = 0;
    std::vector<int> visibleIdx;                       // snapshot indices of visible dots
    std::vector<juce::Point<float>> screenPos;         // their screen positions
    static constexpr int kCell = 12;
    int gridW = 0, gridH = 0;
    std::vector<std::vector<int>> grid;                // cell -> positions in visibleIdx

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapView)
};
} // namespace sk
