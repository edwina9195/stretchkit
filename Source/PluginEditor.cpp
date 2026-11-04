#include "PluginEditor.h"

namespace sk
{
//==============================================================================
void MidiDragSource::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    if (t.chromeIcons)
    {
        const auto img = skinpaint::chromeIcon ("midi");
        const float s = r.getHeight() + 2.0f;
        const auto box = r.removeFromLeft (s).withSizeKeepingCentre (s, s);
        if (isMouseOver())
        {
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.25f), box.getCentre(),
                                                     juce::Colours::white.withAlpha (0.0f), box.getCentre().translated (s * 0.6f, 0.0f), true));
            g.fillEllipse (box.expanded (4.0f));
        }
        g.setColour (juce::Colour (0xff10104a).withAlpha (0.35f));
        g.drawImage (img, box.translated (1.5f, 2.5f), juce::RectanglePlacement::centred, true);
        g.setOpacity (1.0f);
        g.drawImage (img, box, juce::RectanglePlacement::centred);
        g.setColour (t.text);
        g.setFont (t.font (Theme::Role::UIBold, 14.0f));
        g.drawText ("DRAG MIDI", r.withTrimmedLeft (3.0f), juce::Justification::centredLeft);
        return;
    }

    if (auto* l = dynamic_cast<SkLookAndFeel*> (&getLookAndFeel()))
        l->drawSkinButton (g, r, t.accent, false, isMouseOver(), dragging);

    if (t.buttons == ButtonStyle::Pixel)
        r = r.withTrimmedRight (3.0f).withTrimmedBottom (3.0f);

    g.setColour (t.buttonText);
    g.setFont (t.font (Theme::Role::UIBold, 14.0f));
    g.drawText ("Drag MIDI", r, juce::Justification::centred);
}

void MidiDragSource::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4)
        return;

    dragging = true;
    const auto file = processor.exportPatternAsMidi();
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                    [this] { dragging = false; });
}
} // namespace sk

//==============================================================================
using namespace sk;

namespace
{
void blob (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour col, float alpha)
{
    g.setGradientFill (juce::ColourGradient (col.withAlpha (alpha), c, col.withAlpha (0.0f), c.translated (radius, 0.0f), true));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
}
} // namespace

StretchKitEditor::StretchKitEditor (StretchKitProcessor& p)
    : AudioProcessorEditor (p), processor (p), midiDrag (p)
{
    setLookAndFeel (&lnf);

    // ---- header
    addFolderButton.setTooltip ("Add a folder of drum samples. It is scanned, sorted by sound and placed on the map.");
    addFolderButton.onClick = [this] { chooseFolder(); };
    auto setIcon = [] (juce::Button& b, const char* icon, const char* layout)
    {
        b.getProperties().set ("icon", icon);
        b.getProperties().set ("iconLayout", layout);
    };
    setIcon (addFolderButton, "add", "inline");
    setIcon (foldersButton, "folder", "inline");
    setIcon (rescanButton, "rescan", "inline");
    setIcon (playButton, "play", "inline");
    foldersButton.onClick = [this] { showFolderMenu(); };
    rescanButton.setTooltip ("Look for new or changed samples");
    rescanButton.onClick = [this] { processor.library->rescan(); };
    addAndMakeVisible (addFolderButton);
    addAndMakeVisible (foldersButton);
    addAndMakeVisible (rescanButton);

    skinLabel.setText ("SKIN", juce::dontSendNotification);
    skinLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (skinLabel);
    for (int i = 0; i < kNumThemes; ++i)
        skinBox.addItem (Theme::get (i).name, i + 1);
    skinBox.setTooltip ("Change the look of the plugin. Your choice is also used for new instances.");
    skinBox.onChange = [this] { applyTheme ((ThemeId) (skinBox.getSelectedId() - 1)); };
    addAndMakeVisible (skinBox);

    for (int c = 0; c < kNumCategories; ++c)
    {
        auto& b = categoryButtons[(size_t) c];
        const auto cat = (Category) c;
        b.setButtonText (categoryName (cat));
        b.setClickingTogglesState (true);
        b.setToggleState (true, juce::dontSendNotification);
        b.setTooltip ("Show / hide " + juce::String (categoryName (cat)) + " on the map");
        static const char* icons[kNumCategories] = { "kick", "snare", "clap", "hat", "cymbal", "tom", "perc", "other" };
        b.getProperties().set ("icon", icons[c]);
        b.getProperties().set ("iconLayout", "stack");
        b.onClick = [this, c] { map.setCategoryVisible ((Category) c, categoryButtons[(size_t) c].getToggleState()); };
        addAndMakeVisible (b);
    }

    // ---- map
    map.onPick   = [this] (int index, bool fromScrub) { pickSample (index, fromScrub); };
    map.onAssign = [this] (const juce::String& path) { processor.setTrackSample (selectedTrack, path); };
    map.setTooltip ("Click: listen   Click and drag: hear every sound you pass   Drag out onto a track: load it\n"
                    "Double-click: load into the selected track   Wheel / pinch: zoom   Cmd-drag or right-drag: move");
    addAndMakeVisible (map);

    searchBox.setTextToShowWhenEmpty ("Search", juce::Colours::grey);
    searchBox.setTooltip ("Show only sounds whose file name contains these words");
    searchBox.onTextChange = [this] { applyFilter(); };
    searchBox.onEscapeKey  = [this] { searchBox.clear(); applyFilter(); };
    addAndMakeVisible (searchBox);

    favButton.setClickingTogglesState (true);
    favButton.setTooltip ("Show only the sounds you marked with the heart");
    favButton.onClick = [this] { applyFilter(); };
    addAndMakeVisible (favButton);

    simBar.onAudition = [this] (int index)
    {
        if (shownSnapshot != nullptr && juce::isPositiveAndBelow (index, (int) shownSnapshot->size()))
            processor.previewFile ((*shownSnapshot)[(size_t) index].path);
    };
    simBar.onSeedChanged = [this] (int index)
    {
        if (shownSnapshot != nullptr && juce::isPositiveAndBelow (index, (int) shownSnapshot->size()))
            seedPath = (*shownSnapshot)[(size_t) index].path;
    };
    simBar.onSelectionChanged = [this] { updateMapSelection(); };
    simBar.isFavourite  = [this] (const juce::String& path) { return processor.library->isFavourite (path); };
    simBar.setFavourite = [this] (const juce::String& path, bool f) { processor.library->setFavourite (path, f); };
    simBar.onLoad = [this] (const juce::String& path) { processor.setTrackSample (selectedTrack, path); processor.auditionTrack (selectedTrack); };
    addAndMakeVisible (simBar);

    // ---- transport
    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupLabel (bpmLabel, "BPM");
    setupLabel (stepsLabel, "Steps");
    setupLabel (rateLabel, "Rate");
    setupLabel (swingLabel, "Swing");
    setupLabel (modeLabel, "Stretch");
    setupLabel (masterLabel, "Master");

    playButton.setClickingTogglesState (true);
    playButton.setTooltip ("Play the sequencer when the DAW is stopped (follows the DAW when it plays)");
    playButton.onClick = [this] { processor.setInternalPlaying (playButton.getToggleState()); };
    addAndMakeVisible (playButton);

    for (auto* s : { &bpmSlider, &swingSlider, &masterSlider })
    {
        s->setSliderStyle (juce::Slider::LinearBar);
        s->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 0, 0);
        addAndMakeVisible (*s);
    }
    bpmSlider.setTooltip ("Tempo used when the DAW is not providing one (Standalone)");
    swingSlider.setDoubleClickReturnValue (true, 0.0);
    masterSlider.setDoubleClickReturnValue (true, 0.0);

    stepsBox.addItemList (StretchKitProcessor::stepCountNames(), 1);
    rateBox.addItemList (StretchKitProcessor::rateNames(), 1);
    modeBox.addItemList (StretchKitProcessor::modeNames(), 1);
    modeBox.setTooltip ("Hybrid: attack untouched, tail stretched (best for drums)\n"
                        "Smooth: whole sample stretched\n"
                        "Tape: speed change - pitch follows length");
    for (auto* b : { &stepsBox, &rateBox, &modeBox })
        addAndMakeVisible (*b);

    chokeButton.setTooltip ("A new hit on a track fades out the previous one on the same track");
    addAndMakeVisible (chokeButton);
    addAndMakeVisible (midiDrag);

    bpmAtt    = std::make_unique<SliderAttachment> (processor.apvts, "bpm", bpmSlider);
    swingAtt  = std::make_unique<SliderAttachment> (processor.apvts, "swing", swingSlider);
    masterAtt = std::make_unique<SliderAttachment> (processor.apvts, "master", masterSlider);
    stepsAtt  = std::make_unique<ComboAttachment> (processor.apvts, "steps", stepsBox);
    rateAtt   = std::make_unique<ComboAttachment> (processor.apvts, "rate", rateBox);
    modeAtt   = std::make_unique<ComboAttachment> (processor.apvts, "mode", modeBox);
    chokeAtt  = std::make_unique<ButtonAttachment> (processor.apvts, "choke", chokeButton);

    // ---- tracks
    for (int t = 0; t < kNumTracks; ++t)
    {
        auto row = std::make_unique<TrackRow> (processor, t, *this);
        row->onSelect = [this] (int i) { selectTrack (i); };
        addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }
    selectTrack (0);

    processor.library->addChangeListener (this);
    shownSnapshot = processor.library->getSnapshot();
    map.setSnapshot (shownSnapshot);

    applyTheme ((ThemeId) processor.getThemeIndex());
    updateStatus();

    setResizable (true, true);
    setResizeLimits (1000, 780, 2800, 1900);
    const int w = processor.getEditorWidth(), h = processor.getEditorHeight();
    setSize (w >= 1000 ? w : 1180, h >= 780 ? h : 900);
    startTimerHz (30);
}

StretchKitEditor::~StretchKitEditor()
{
    stopTimer();
    processor.library->removeChangeListener (this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void StretchKitEditor::applyTheme (ThemeId id)
{
    lnf.setTheme (id);
    processor.setThemeIndex ((int) id);
    const auto& t = lnf.theme();

    skinBox.setSelectedId ((int) id + 1, juce::dontSendNotification);
    StretchKitProcessor::saveDefaultSkin ((int) id);
    skinLabel.setFont (t.font (Theme::Role::Label, 12.0f));

    const bool lightSkin = id == ThemeId::Y2KChrome;
    searchBox.setFont (t.font (Theme::Role::UI, 15.0f));
    searchBox.setIndents (12, 4);
    searchBox.setColour (juce::TextEditor::backgroundColourId,
                         lightSkin ? juce::Colours::white.withAlpha (0.85f)
                                   : id == ThemeId::Liquid ? juce::Colour (0xff1d1e6e).withAlpha (0.7f) : t.panel.darker (0.2f));
    searchBox.setColour (juce::TextEditor::textColourId, t.text);
    searchBox.setColour (juce::TextEditor::outlineColourId, t.panelEdge.withAlpha (0.6f));
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, t.accent);
    searchBox.setColour (juce::CaretComponent::caretColourId, t.accent);
    searchBox.setTextToShowWhenEmpty ("Search", t.textDim);
    searchBox.applyFontToAllText (t.font (Theme::Role::UI, 15.0f));
    skinLabel.setColour (juce::Label::textColourId, t.textDim);

    for (auto* l : { &bpmLabel, &stepsLabel, &rateLabel, &swingLabel, &modeLabel, &masterLabel })
    {
        l->setFont (t.font (Theme::Role::Label, 12.0f));
        l->setColour (juce::Label::textColourId, t.textDim);
    }

    for (int c = 0; c < kNumCategories; ++c)
    {
        auto& b = categoryButtons[(size_t) c];
        const auto col = t.category ((Category) c);
        b.setColour (juce::TextButton::buttonOnColourId, col);
        b.setColour (juce::TextButton::textColourOnId, t.onButtonText (col));
        b.setColour (juce::TextButton::textColourOffId, t.buttons == ButtonStyle::Chrome ? t.buttonText.withAlpha (0.6f)
                                                                                        : t.textDim);
    }

    sendLookAndFeelChange();
    resized();
    repaint();
}

//==============================================================================
void StretchKitEditor::paintBackground (juce::Graphics& g, const Theme& t)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();

    g.setGradientFill (juce::ColourGradient (t.bgTop, 0.0f, 0.0f, t.bgBottom, 0.0f, h, false));
    g.fillAll();

    if (t.lightStreak)
        skinpaint::lightStreak (g, b, 0.75f);

    if (t.iridescent)
    {
        blob (g, { w * 0.12f, h * 0.08f }, w * 0.42f, juce::Colour (0xffffc2ea), 0.65f);
        blob (g, { w * 0.88f, h * 0.30f }, w * 0.45f, juce::Colour (0xffaeeeff), 0.70f);
        blob (g, { w * 0.45f, h * 0.62f }, w * 0.40f, juce::Colour (0xffdcd0ff), 0.55f);
        blob (g, { w * 0.70f, h * 1.02f }, w * 0.40f, juce::Colour (0xffe6ffb3), 0.55f);
    }

    if (t.fog)
    {
        blob (g, { w * 0.18f, h * 1.00f }, w * 0.55f, juce::Colour (0xff1d4bb8), 0.30f);
        blob (g, { w * 0.88f, h * 0.92f }, w * 0.45f, juce::Colour (0xff2a5fd8), 0.20f);
        blob (g, { w * 0.50f, h * 0.38f }, w * 0.60f, juce::Colour (0xff16306e), 0.28f);
    }

    if (t.pixelGrid)
    {
        g.setColour (t.accent2.withAlpha (0.035f));
        for (float x = 0.0f; x < w; x += 8.0f) g.fillRect (x, 0.0f, 1.0f, h);
        for (float y = 0.0f; y < h; y += 8.0f) g.fillRect (0.0f, y, w, 1.0f);

        juce::Random rng (7);
        const juce::Colour specks[] = { t.accent, t.accent2, juce::Colour (0xffb6ff00) };
        for (int i = 0; i < 700; ++i)
        {
            g.setColour (specks[i % 3].withAlpha (0.1f + 0.25f * rng.nextFloat()));
            g.fillRect ((float) rng.nextInt ((int) w), (float) rng.nextInt ((int) h), 1.0f, 1.0f);
        }
    }

    if (t.orbitRings)
    {
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        juce::Path ring;
        ring.addEllipse (juce::Rectangle<float> (184.0f, 26.0f).withCentre (titleArea.toFloat().getCentre().translated (8.0f, 2.0f)));
        ring.applyTransform (juce::AffineTransform::rotation (-0.10f, (float) titleArea.getCentreX(), (float) titleArea.getCentreY()));
        g.strokePath (ring, juce::PathStrokeType (1.2f));
    }

    // header divider
    const float divY = (float) headerArea.getBottom() + 3.0f;
    switch (t.id)
    {
        case ThemeId::PS2Night:
            g.setColour (t.accent.withAlpha (0.35f));
            g.fillRect (12.0f, divY, w - 24.0f, 1.0f);
            break;
        case ThemeId::Y2KChrome:
            g.setColour (juce::Colours::white);
            g.fillRect (12.0f, divY, w - 24.0f, 1.5f);
            g.setColour (juce::Colour (0xff9aa7c0).withAlpha (0.6f));
            g.fillRect (12.0f, divY + 1.5f, w - 24.0f, 1.0f);
            break;
        case ThemeId::Digicore:
            g.setGradientFill (juce::ColourGradient (t.accent, 0.0f, 0.0f, t.accent2, w, 0.0f, false));
            g.fillRect (0.0f, divY, w, 2.0f);
            break;
        case ThemeId::Mix:
            g.setGradientFill (juce::ColourGradient (t.accent.withAlpha (0.7f), 0.0f, 0.0f, t.accent2.withAlpha (0.7f), w, 0.0f, false));
            g.fillRect (12.0f, divY, w - 24.0f, 1.0f);
            break;
        case ThemeId::Liquid:
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.fillRect (12.0f, divY, w - 24.0f, 1.0f);
            break;
    }

    // transport bar plate
    const auto plate = transportArea.toFloat().expanded (6.0f, 3.0f);
    switch (t.id)
    {
        case ThemeId::PS2Night:
        case ThemeId::Mix:
            g.setColour (juce::Colour (0xff0a1534).withAlpha (0.6f));
            g.fillRoundedRectangle (plate, 8.0f);
            g.setColour (t.panelEdge.withAlpha (0.25f));
            g.drawRoundedRectangle (plate, 8.0f, 1.0f);
            break;
        case ThemeId::Liquid:
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (plate, plate.getHeight() * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawRoundedRectangle (plate, plate.getHeight() * 0.5f, 1.0f);
            break;
        case ThemeId::Y2KChrome:
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillRoundedRectangle (plate, plate.getHeight() * 0.5f);
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (plate, plate.getHeight() * 0.5f, 1.2f);
            break;
        case ThemeId::Digicore:
        {
            g.setColour (t.panel.withAlpha (0.9f));
            g.fillRect (plate);
            juce::Path border;
            border.addRectangle (plate);
            juce::Path dashed;
            const float dashes[] = { 4.0f, 3.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, border, dashes, 2);
            g.setColour (t.accent2.withAlpha (0.6f));
            g.fillPath (dashed);
            break;
        }
    }

    // column titles above the tracks
    g.setColour (t.textDim);
    g.setFont (t.font (Theme::Role::Label, 11.0f));
    auto r = trackHeaderArea;
    g.drawText ("TRACK (drop samples here)", r.removeFromLeft (182), juce::Justification::centredLeft);
    r.removeFromLeft (66 + 4);
    g.drawText ("VOL", r.removeFromLeft (36), juce::Justification::centred);
    g.drawText ("PAN", r.removeFromLeft (36), juce::Justification::centred);
    r.removeFromLeft (4);
    g.drawText ("PITCH", r.removeFromLeft (70), juce::Justification::centred);
    g.drawText ("LENGTH %", r.removeFromLeft (80), juce::Justification::centred);
    r.removeFromLeft (8);
    g.drawText ("STEPS   click: on/off   right-click / shift: accent", r, juce::Justification::centredLeft);
}

void StretchKitEditor::paintTitle (juce::Graphics& g, const Theme& t)
{
    const auto r = titleArea.toFloat();
    const juce::String text ("STRETCHKIT");

    switch (t.id)
    {
        case ThemeId::PS2Night:
        {
            const auto f = t.font (Theme::Role::Title, 22.0f);
            g.setFont (f);
            for (float d : { -1.5f, 1.5f })
            {
                g.setColour (t.accent.withAlpha (0.35f));
                g.drawText (text, r.translated (d, 0.0f), juce::Justification::centredLeft, false);
                g.drawText (text, r.translated (0.0f, d), juce::Justification::centredLeft, false);
            }
            g.setColour (t.text);
            g.drawText (text, r, juce::Justification::centredLeft, false);
            break;
        }

        case ThemeId::Y2KChrome:
        case ThemeId::Mix:
        case ThemeId::Liquid:
        {
            juce::Path p;
            skinpaint::textPath (p, text, t.font (Theme::Role::Title, t.id == ThemeId::Y2KChrome ? 18.0f : 20.0f), r,
                                 juce::Justification::centredLeft);
            const auto pb = p.getBounds();

            if (t.glitch)
            {
                g.setColour (juce::Colour (0xff00f0ff).withAlpha (0.55f));
                g.fillPath (p, juce::AffineTransform::translation (-1.5f, 0.0f));
                g.setColour (juce::Colour (0xffff2bd6).withAlpha (0.55f));
                g.fillPath (p, juce::AffineTransform::translation (1.5f, 0.0f));
            }
            else
            {
                g.setColour (juce::Colour (0xff18203d).withAlpha (0.3f));   // soft drop shadow
                g.fillPath (p, juce::AffineTransform::translation (1.0f, 1.5f));
            }
            skinpaint::chromeFill (g, p, pb, true);
            if (t.id == ThemeId::Y2KChrome)
            {
                g.setColour (juce::Colour (0xff5d677d));
                g.strokePath (p, juce::PathStrokeType (0.6f));
            }

            skinpaint::sparkle (g, { pb.getRight() + 6.0f, pb.getY() - 1.0f }, 7.0f,
                                t.id == ThemeId::Mix ? t.accent : juce::Colours::white);
            skinpaint::sparkle (g, { pb.getX() - 2.0f, pb.getBottom() + 1.0f }, 4.5f, t.accent2);
            break;
        }

        case ThemeId::Digicore:
        {
            skinpaint::glitchText (g, text, t.font (Theme::Role::Title, 18.0f), r, juce::Justification::centredLeft,
                                   juce::Colours::white, 2.0f);
            skinpaint::sparkle (g, { r.getRight() - 4.0f, r.getY() + 6.0f }, 6.0f, juce::Colour (0xffb6ff00));
            skinpaint::sparkle (g, { r.getRight() + 6.0f, r.getBottom() - 6.0f }, 4.0f, t.accent2);
            break;
        }
    }
}

void StretchKitEditor::paint (juce::Graphics& g)
{
    const auto& t = lnf.theme();
    paintBackground (g, t);
    paintTitle (g, t);

    if (t.menuLights)
        skinpaint::menuLights (g, { (float) chokeButton.getRight() + 16.0f, (float) chokeButton.getBounds().getCentreY() }, 14.0f);

    if (t.sparkles && t.id != ThemeId::Digicore)
    {
        skinpaint::sparkle (g, { (float) getWidth() - 18.0f, (float) transportArea.getY() - 6.0f }, 5.0f, t.accent2);
        skinpaint::sparkle (g, { 22.0f, (float) tracksArea.getBottom() + 2.0f }, 4.0f, t.accent);
    }
    if (t.id == ThemeId::Digicore)
    {
        juce::Random rng (11);
        for (int i = 0; i < 9; ++i)
        {
            const juce::Colour cols[] = { t.accent, t.accent2, juce::Colour (0xffb6ff00) };
            skinpaint::sparkle (g, { 8.0f + rng.nextFloat() * ((float) getWidth() - 16.0f),
                                     i % 2 == 0 ? (float) transportArea.getY() - 7.0f : (float) getHeight() - 5.0f },
                                3.0f + 3.0f * rng.nextFloat(), cols[i % 3]);
        }
    }
}

void StretchKitEditor::paintOverChildren (juce::Graphics& g)
{
    const auto& t = lnf.theme();
    if (! t.scanlines)
        return;

    const float alpha = t.id == ThemeId::Digicore ? 0.16f : t.id == ThemeId::Mix ? 0.07f : 0.06f;
    g.setColour (juce::Colours::black.withAlpha (alpha));
    const auto clip = g.getClipBounds();
    for (int y = clip.getY() - clip.getY() % 3; y < clip.getBottom(); y += 3)
        g.fillRect (clip.getX(), y, clip.getWidth(), 1);
}

void StretchKitEditor::resized()
{
    const auto& t = lnf.theme();
    const bool icons = t.chromeIcons;
    auto area = getLocalBounds().reduced (12, 8);

    auto header = area.removeFromTop (icons ? 50 : 34);
    headerArea = header;
    titleArea = header.removeFromLeft (168).withSizeKeepingCentre (168, 34);

    auto place = [&] (juce::Button& b, int w, int wIcon)
    {
        b.setBounds (header.removeFromLeft (icons ? wIcon : w).withSizeKeepingCentre (icons ? wIcon : w, 34).reduced (2, 3));
    };
    place (addFolderButton, 96, 120);
    place (foldersButton, 76, 100);
    place (rescanButton, 72, 96);

    for (int c = kNumCategories; --c >= 0;)
        categoryButtons[(size_t) c].setBounds (icons ? header.removeFromRight (56).reduced (1, 0)
                                                     : header.removeFromRight (62).reduced (2, 4).withSizeKeepingCentre (58, 26));
    header.removeFromLeft (6);
    skinLabel.setBounds (header.removeFromLeft (40));
    skinBox.setBounds (header.removeFromLeft (132).withSizeKeepingCentre (132, 26));

    area.removeFromTop (8);
    const int rowH = 44;
    const int below = 6 + 42 + 12 + 32 + 8 + 18 + rowH * kNumTracks;
    auto mapBounds = area.removeFromTop (juce::jmax (220, area.getHeight() - below));
    map.setBounds (mapBounds);
    const int searchW = juce::jmin (320, mapBounds.getWidth() / 3);
    searchBox.setBounds (mapBounds.getCentreX() - searchW / 2, mapBounds.getY() + (t.id == ThemeId::Digicore ? 24 : 8), searchW, 26);
    favButton.setBounds (searchBox.getRight() + 8, searchBox.getY(), 100, 26);
    area.removeFromTop (6);
    simBar.setBounds (area.removeFromTop (42));
    area.removeFromTop (12);

    auto transport = area.removeFromTop (32);
    transportArea = transport;
    playButton.setBounds (transport.removeFromLeft (icons ? 78 : 64).reduced (2, 2));
    bpmLabel.setBounds (transport.removeFromLeft (icons ? 50 : 64));
    bpmSlider.setBounds (transport.removeFromLeft (80).reduced (2, 4));
    stepsLabel.setBounds (transport.removeFromLeft (52));
    stepsBox.setBounds (transport.removeFromLeft (60).reduced (2, 3));
    rateLabel.setBounds (transport.removeFromLeft (46));
    rateBox.setBounds (transport.removeFromLeft (74).reduced (2, 3));
    swingLabel.setBounds (transport.removeFromLeft (54));
    swingSlider.setBounds (transport.removeFromLeft (80).reduced (2, 4));
    modeLabel.setBounds (transport.removeFromLeft (64));
    modeBox.setBounds (transport.removeFromLeft (92).reduced (2, 3));
    chokeButton.setBounds (transport.removeFromLeft (80).reduced (8, 2));
    midiDrag.setBounds (transport.removeFromRight (icons ? 116 : 96).reduced (2, 2));
    masterSlider.setBounds (transport.removeFromRight (84).reduced (2, 4));
    masterLabel.setBounds (transport.removeFromRight (60));

    area.removeFromTop (8);
    trackHeaderArea = area.removeFromTop (18);
    tracksArea = area;
    for (auto& row : rows)
        row->setBounds (area.removeFromTop (rowH));
}

//==============================================================================
void StretchKitEditor::selectTrack (int index)
{
    selectedTrack = index;
    for (int t = 0; t < (int) rows.size(); ++t)
        rows[(size_t) t]->setSelected (t == index);
    map.setHighlightedPath (processor.getTrackPath (index));
}

void StretchKitEditor::chooseFolder()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a folder with drum samples",
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory));
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto dir = fc.getResult();
                              if (dir.isDirectory())
                                  processor.library->addFolder (dir);
                          });
}

void StretchKitEditor::showFolderMenu()
{
    juce::PopupMenu menu;
    const auto folders = processor.library->getFolders();
    if (folders.isEmpty())
        menu.addItem (-1, "No folders yet", false);

    menu.addSectionHeader ("Click a folder to remove it");
    for (int i = 0; i < folders.size(); ++i)
        menu.addItem (i + 1, folders[i]);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&foldersButton),
                        [this, folders] (int result)
                        {
                            if (result > 0 && result <= folders.size())
                                processor.library->removeFolder (folders[result - 1]);
                        });
}

void StretchKitEditor::updateStatus()
{
    auto& lib = *processor.library;
    juce::String text;
    if (lib.isScanning())
        text << "Scanning " << lib.getScannedCount() << " / " << lib.getTotalToScan() << " ...";
    else
        text << (int) lib.getSnapshot()->size() << " samples on the map";
    map.setStatusText (text);
}

void StretchKitEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    const auto snap = processor.library->getSnapshot();
    if (snap != shownSnapshot)
    {
        shownSnapshot = snap;
        map.setSnapshot (snap);

        // keep the similarity seed when the library is refreshed
        int seedIndex = -1;
        if (seedPath.isNotEmpty())
            for (int i = 0; i < (int) snap->size(); ++i)
                if ((*snap)[(size_t) i].path == seedPath) { seedIndex = i; break; }
        if (seedIndex >= 0) simBar.setSeed (snap, seedIndex, false);
        else                simBar.clear();
    }
    map.setHighlightedPath (processor.getTrackPath (selectedTrack));
    updateStatus();
}

void StretchKitEditor::pickSample (int index, bool)
{
    if (shownSnapshot == nullptr || ! juce::isPositiveAndBelow (index, (int) shownSnapshot->size()))
        return;
    processor.previewFile ((*shownSnapshot)[(size_t) index].path);
    seedPath = (*shownSnapshot)[(size_t) index].path;
    simBar.setSeed (shownSnapshot, index, true);
}

void StretchKitEditor::updateMapSelection()
{
    map.setSelection (simBar.getSeed(), simBar.getNeighbours(), simBar.getCurrent());
}

void StretchKitEditor::applyFilter()
{
    map.setFilter (searchBox.getText(), favButton.getToggleState(),
                   [lib = &processor.library.getObject()] (const juce::String& p) { return lib->isFavourite (p); });
}

void StretchKitEditor::timerCallback()
{
    for (auto& row : rows)
        row->refresh();

    const bool host = processor.isHostPlaying();
    playButton.setButtonText (host ? "Host" : processor.isInternalPlaying() ? "Stop" : "Play");
    playButton.getProperties().set ("icon", ! host && processor.isInternalPlaying() ? "stop" : "play");
    playButton.setToggleState (host || processor.isInternalPlaying(), juce::dontSendNotification);

    const auto bpmText = host ? "Host " + juce::String (processor.getDisplayBpm(), 1) : juce::String ("BPM");
    if (bpmLabel.getText() != bpmText)
        bpmLabel.setText (bpmText, juce::dontSendNotification);

    if (++statusTick % 10 == 0)
        updateStatus();

    processor.setEditorSize (getWidth(), getHeight());

    map.setHighlightedPath (processor.getTrackPath (selectedTrack));
}
