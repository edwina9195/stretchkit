#pragma once

#include "PluginProcessor.h"
#include "ui/MapView.h"
#include "ui/SimilarityBar.h"
#include "ui/Theme.h"
#include "ui/TrackRow.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace sk
{
/** Drag this into the DAW to get the current pattern as a MIDI clip. */
class MidiDragSource : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    explicit MidiDragSource (StretchKitProcessor& p) : processor (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Drag the pattern into your DAW as a MIDI clip (notes C1 - G1)");
    }

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    StretchKitProcessor& processor;
    bool dragging = false;
};
} // namespace sk

class StretchKitEditor : public juce::AudioProcessorEditor,
                         public juce::DragAndDropContainer,
                         private juce::ChangeListener,
                         private juce::Timer
{
public:
    explicit StretchKitEditor (sk::StretchKitProcessor&);
    ~StretchKitEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    /** Listens to a library sound and makes it the similarity seed (what a map click does). */
    void pickSample (int index, bool fromScrub);

    /** Switches the skin (also remembered in the plugin state). */
    void applyTheme (sk::ThemeId id);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void selectTrack (int index);
    void chooseFolder();
    void showFolderMenu();
    void updateStatus();
    void updateMapSelection();
    void applyFilter();
    void paintBackground (juce::Graphics&, const sk::Theme&);
    void paintTitle (juce::Graphics&, const sk::Theme&);

    sk::StretchKitProcessor& processor;
    sk::SkLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    // header
    juce::TextButton addFolderButton { "Add Folder" }, foldersButton { "Folders" }, rescanButton { "Rescan" };
    juce::Label skinLabel;
    juce::ComboBox skinBox;
    std::array<juce::TextButton, sk::kNumCategories> categoryButtons;

    // map
    sk::MapView map;
    juce::TextEditor searchBox;
    juce::TextButton favButton { "Favourites" };
    sk::SimilarityBar simBar;
    sk::LibrarySnapshot shownSnapshot;
    juce::String seedPath;

    // transport
    juce::TextButton playButton { "Play" };
    juce::Label bpmLabel, stepsLabel, rateLabel, swingLabel, modeLabel, masterLabel;
    juce::Slider bpmSlider, swingSlider, masterSlider;
    juce::ComboBox stepsBox, rateBox, modeBox;
    juce::ToggleButton chokeButton { "Choke" };
    sk::MidiDragSource midiDrag;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> bpmAtt, swingAtt, masterAtt;
    std::unique_ptr<ComboAttachment> stepsAtt, rateAtt, modeAtt;
    std::unique_ptr<ButtonAttachment> chokeAtt;

    // tracks
    std::vector<std::unique_ptr<sk::TrackRow>> rows;
    int selectedTrack = 0;
    int statusTick = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> titleArea, headerArea, transportArea, trackHeaderArea, tracksArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StretchKitEditor)
};
