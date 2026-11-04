#pragma once

#include "library/SampleLibrary.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace sk
{
/** The sample map: every scanned one-shot as a dot, grouped by category.
    Click = audition, drag a dot onto a track = assign, double-click = assign
    to the selected track, wheel = zoom, drag empty space = pan,
    hold Alt and move = audition everything under the cursor. */
class MapView : public juce::Component,
                public juce::SettableTooltipClient
{
public:
    MapView();

    void setSnapshot (LibrarySnapshot snap);
    void setCategoryVisible (Category c, bool visible);
    bool isCategoryVisible (Category c) const { return visible[(size_t) c]; }
    void setHighlightedPath (const juce::String& path);
    void setStatusText (const juce::String& text);

    std::function<void (const juce::String&)> onPreview;
    std::function<void (const juce::String&)> onAssign;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> mapArea() const;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> fromScreen (juce::Point<float> p) const;
    int hitTestDot (juce::Point<float> p) const;
    float dotRadius() const;

    LibrarySnapshot snapshot;
    std::array<bool, kNumCategories> visible;
    int hovered = -1, pressed = -1;
    bool dragStarted = false, panning = false;
    juce::String highlightedPath, statusText;

    float zoom = 1.0f;
    juce::Point<float> centre { 0.5f, 0.5f }, panStartCentre;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapView)
};
} // namespace sk
