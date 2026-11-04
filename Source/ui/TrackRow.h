#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace sk
{
/** Step buttons for one track. Click = on/off, right-click or Shift-click = accent,
    drag across to paint. */
class StepGrid : public juce::Component
{
public:
    StepGrid (StretchKitProcessor& p, int trackIndex) : processor (p), track (trackIndex) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    /** Returns true when something changed and a repaint happened. */
    bool refresh();

private:
    int cellAt (juce::Point<float> p) const;
    void apply (int cell);

    StretchKitProcessor& processor;
    const int track;
    int paintValue = 0, lastCell = -1, shownStep = -2, shownSteps = 0;
};

/** One sequencer track: sample name, dice, mute, volume, pan, pitch, length %, steps.
    Accepts dots dragged from the map and audio files dragged from the desktop. */
class TrackRow : public juce::Component,
                 public juce::DragAndDropTarget,
                 public juce::FileDragAndDropTarget
{
public:
    TrackRow (StretchKitProcessor& p, int trackIndex, juce::Component& popupParent);

    std::function<void (int)> onSelect;

    void setSelected (bool shouldBeSelected);
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override   { setDropHover (true); }
    void itemDragExit (const SourceDetails&) override    { setDropHover (false); }
    void itemDropped (const SourceDetails&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { setDropHover (true); }
    void fileDragExit (const juce::StringArray&) override            { setDropHover (false); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void setDropHover (bool);
    juce::Rectangle<int> nameArea() const;

    StretchKitProcessor& processor;
    const int index;

    juce::TextButton diceButton { "Rnd" }, muteButton { "M" };
    juce::Slider volume, pan, pitch, length;
    StepGrid grid;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> volAtt, panAtt, pitchAtt, lenAtt;
    std::unique_ptr<ButtonAttachment> muteAtt;

    bool selected = false, dropHover = false, rendering = false;
    juce::String shownName;
    Category shownCategory = Category::Other;
    int lastHits = 0;
    float flash = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRow)
};
} // namespace sk
