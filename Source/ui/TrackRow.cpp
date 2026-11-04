#include "TrackRow.h"
#include "Theme.h"

namespace sk
{
//==============================================================================
int StepGrid::cellAt (juce::Point<float> p) const
{
    const int count = processor.getNumSteps();
    const float w = (float) getWidth() / (float) count;
    return juce::jlimit (0, count - 1, (int) (p.x / w));
}

void StepGrid::apply (int cell)
{
    if (cell != lastCell)
    {
        processor.setStep (track, cell, paintValue);
        lastCell = cell;
        repaint();
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.position);
    const int cur = processor.getStep (track, cell);

    if (e.mods.isPopupMenu() || e.mods.isShiftDown())
        paintValue = cur == 2 ? 1 : 2;
    else
        paintValue = cur != 0 ? 0 : 1;

    lastCell = -1;
    apply (cell);
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    apply (cellAt (e.position));
}

bool StepGrid::refresh()
{
    const int step = processor.getCurrentStep();
    const int count = processor.getNumSteps();
    if (step != shownStep || count != shownSteps)
    {
        shownStep = step;
        shownSteps = count;
        repaint();
        return true;
    }
    return false;
}

void StepGrid::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    const int count = processor.getNumSteps();
    const float w = (float) getWidth() / (float) count;
    const float h = (float) getHeight();
    const auto colour = t.category (processor.getTrackCategory (track));
    const int playing = processor.getCurrentStep();

    for (int s = 0; s < count; ++s)
    {
        const auto slot = juce::Rectangle<float> ((float) s * w, 0.0f, w, h);
        const auto cell = slot.reduced (juce::jmin (2.5f, w * 0.12f), 5.0f);
        const int v = processor.getStep (track, s);
        const auto off = (s / 4) % 2 == 0 ? t.cellOff : t.cellOffAlt;
        const bool isPlaying = s == playing;

        switch (t.cells)
        {
            case CellStyle::GlassCube:
            {
                const float rad = 3.0f;
                if (isPlaying)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.06f));
                    g.fillRect (slot);
                }
                if (v != 0)
                {
                    g.setColour (colour.withAlpha (v == 2 ? 0.35f : 0.2f));
                    g.fillRoundedRectangle (cell.expanded (2.5f), rad + 2.0f);
                    g.setGradientFill (juce::ColourGradient (colour.brighter (0.35f).withAlpha (v == 2 ? 1.0f : 0.75f), 0.0f, cell.getY(),
                                                             colour.darker (0.45f).withAlpha (v == 2 ? 1.0f : 0.75f), 0.0f, cell.getBottom(), false));
                }
                else
                {
                    g.setGradientFill (juce::ColourGradient (off.brighter (0.25f), 0.0f, cell.getY(), off, 0.0f, cell.getBottom(), false));
                }
                g.fillRoundedRectangle (cell, rad);
                g.setColour (juce::Colours::white.withAlpha (v == 2 ? 0.75f : v == 1 ? 0.35f : 0.09f));
                g.fillRect (cell.reduced (rad, 0.0f).withHeight (v == 2 ? 2.0f : 1.0f).translated (0.0f, 1.5f));
                g.setColour (v != 0 ? colour.brighter (0.4f).withAlpha (0.8f) : juce::Colours::white.withAlpha (0.06f));
                g.drawRoundedRectangle (cell, rad, 1.0f);
                if (isPlaying)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.95f));
                    g.drawRoundedRectangle (cell.expanded (1.0f), rad + 1.0f, 1.5f);
                }
                break;
            }

            case CellStyle::Gel:
            {
                const float rad = juce::jmin (8.0f, cell.getHeight() * 0.32f);
                if (v != 0)
                {
                    const float a = v == 2 ? 1.0f : 0.7f;
                    juce::ColourGradient gel (colour.brighter (0.65f).withAlpha (a), 0.0f, cell.getY(), colour.darker (0.1f).withAlpha (a), 0.0f, cell.getBottom(), false);
                    gel.addColour (0.55, colour.withAlpha (a));
                    g.setGradientFill (gel);
                    g.fillRoundedRectangle (cell, rad);
                    g.setColour (colour.darker (0.5f).withAlpha (0.7f));
                    g.drawRoundedRectangle (cell, rad, 1.0f);
                }
                else if (t.id == ThemeId::Liquid)
                {
                    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), 0.0f, cell.getY(),
                                                             juce::Colours::white.withAlpha (0.05f), 0.0f, cell.getBottom(), false));
                    g.fillRoundedRectangle (cell, rad);
                    g.setColour (juce::Colours::white.withAlpha ((s / 4) % 2 == 0 ? 0.3f : 0.2f));
                    g.drawRoundedRectangle (cell, rad, 1.0f);
                }
                else
                {
                    g.setColour (juce::Colours::white.withAlpha (0.65f));
                    g.fillRoundedRectangle (cell, rad);
                    g.setColour (juce::Colour (0xffb5c0d3));
                    g.drawRoundedRectangle (cell, rad, 1.0f);
                    g.setColour (juce::Colour (0xff9aa7c0).withAlpha (0.25f));
                    g.fillRoundedRectangle (cell.withTrimmedTop (cell.getHeight() * 0.7f).reduced (2.0f, 1.0f), rad * 0.5f);
                }
                const auto gloss = cell.reduced (cell.getWidth() * 0.14f, 2.0f).withHeight (cell.getHeight() * 0.36f);
                g.setColour (juce::Colours::white.withAlpha (v != 0 ? 0.6f : t.id == ThemeId::Liquid ? 0.12f : 0.8f));
                g.fillRoundedRectangle (gloss, gloss.getHeight() * 0.5f);
                if (v == 2)
                    skinpaint::sparkle (g, { cell.getRight() - 5.0f, cell.getY() + 5.0f }, 4.0f, juce::Colours::white);
                if (isPlaying)
                {
                    g.setColour (t.accent);
                    g.drawRoundedRectangle (cell.expanded (1.5f), rad + 1.5f, 2.0f);
                }
                break;
            }

            case CellStyle::Pixel:
            {
                const auto px = cell.toNearestInt().toFloat();
                if (v != 0)
                {
                    g.setColour (colour.withAlpha (v == 2 ? 1.0f : 0.6f));
                    g.fillRect (px);
                    if (v == 1)
                    {
                        g.setColour (juce::Colour (0xff0b0014).withAlpha (0.35f));   // dither
                        for (float yy = px.getY(); yy < px.getBottom(); yy += 2.0f)
                            for (float xx = px.getX() + std::fmod (yy - px.getY(), 4.0f) * 0.5f; xx < px.getRight(); xx += 2.0f)
                                g.fillRect (xx, yy, 1.0f, 1.0f);
                    }
                    else
                    {
                        g.setColour (juce::Colours::white);
                        g.fillRect (px.getX() + 2.0f, px.getY() + 2.0f, 3.0f, 3.0f);
                    }
                }
                else
                {
                    g.setColour (off);
                    g.fillRect (px);
                }
                g.setColour (v != 0 ? colour.brighter (0.5f) : juce::Colour (0xff2c1d46));
                g.drawRect (px, 1.0f);
                if (isPlaying)
                {
                    g.setColour (juce::Colour (0xffb6ff00));
                    g.drawRect (px.expanded (2.0f), 2.0f);
                }
                break;
            }
        }
    }
}

//==============================================================================
TrackRow::TrackRow (StretchKitProcessor& p, int trackIndex, juce::Component& popupParent)
    : processor (p), index (trackIndex), grid (p, trackIndex)
{
    auto setupRotary = [&] (juce::Slider& s, const juce::String& tip)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setPopupDisplayEnabled (true, true, &popupParent);
        s.setTooltip (tip);
        addAndMakeVisible (s);
    };

    auto setupBar = [&] (juce::Slider& s, const juce::String& tip)
    {
        s.setSliderStyle (juce::Slider::LinearBar);
        s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 0, 0);
        s.setTooltip (tip);
        addAndMakeVisible (s);
    };

    setupRotary (volume, "Volume");
    setupRotary (pan, "Pan");
    setupBar (pitch, "Pitch (semitones). Double-click to reset.");
    setupBar (length, "Length relative to the original sample (time-stretch). Double-click = 100%.");

    const auto id = juce::String (trackIndex);
    volAtt   = std::make_unique<SliderAttachment> (processor.apvts, "vol" + id, volume);
    panAtt   = std::make_unique<SliderAttachment> (processor.apvts, "pan" + id, pan);
    pitchAtt = std::make_unique<SliderAttachment> (processor.apvts, "pitch" + id, pitch);
    lenAtt   = std::make_unique<SliderAttachment> (processor.apvts, "len" + id, length);

    // hear the result as soon as a pitch / length change has been rendered
    pitch.onDragEnd  = [this] { processor.auditionTrack (index); };
    length.onDragEnd = [this] { processor.auditionTrack (index); };

    volume.setDoubleClickReturnValue (true, 0.0);
    pan.setDoubleClickReturnValue (true, 0.0);
    pitch.setDoubleClickReturnValue (true, 0.0);
    length.setDoubleClickReturnValue (true, 100.0);

    muteButton.setClickingTogglesState (true);
    muteButton.setTooltip ("Mute");
    muteAtt = std::make_unique<ButtonAttachment> (processor.apvts, "mute" + id, muteButton);
    addAndMakeVisible (muteButton);

    diceButton.setTooltip ("Swap for a similar sound (nearby on the map)");
    diceButton.onClick = [this] { processor.randomiseTrack (index); };
    diceButton.getProperties().set ("icon", "dice");
    diceButton.getProperties().set ("iconLayout", "only");
    addAndMakeVisible (diceButton);

    addAndMakeVisible (grid);
    refresh();
}

juce::Rectangle<int> TrackRow::nameArea() const
{
    return getLocalBounds().withWidth (178).reduced (0, 3);
}

void TrackRow::resized()
{
    auto r = getLocalBounds().reduced (0, 4);
    r.removeFromLeft (182);
    diceButton.setBounds (r.removeFromLeft (38).reduced (2, 2));
    muteButton.setBounds (r.removeFromLeft (28).reduced (2, 2));
    r.removeFromLeft (4);
    volume.setBounds (r.removeFromLeft (36));
    pan.setBounds (r.removeFromLeft (36));
    r.removeFromLeft (4);
    pitch.setBounds (r.removeFromLeft (70).reduced (2, 3));
    length.setBounds (r.removeFromLeft (80).reduced (2, 3));
    r.removeFromLeft (8);
    grid.setBounds (r);
}

void TrackRow::setSelected (bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}

void TrackRow::setDropHover (bool h)
{
    dropHover = h;
    repaint();
}

void TrackRow::refresh()
{
    bool dirty = false;

    const auto name = processor.getTrackName (index);
    const auto cat = processor.getTrackCategory (index);
    const bool r = processor.isTrackRendering (index);
    if (name != shownName || cat != shownCategory || r != rendering)
    {
        shownName = name;
        shownCategory = cat;
        rendering = r;
        dirty = true;
    }

    const int hits = processor.getHitCounter (index);
    if (hits != lastHits)
    {
        lastHits = hits;
        flash = 1.0f;
        dirty = true;
    }
    else if (flash > 0.0f)
    {
        flash = juce::jmax (0.0f, flash - 0.15f);
        dirty = true;
    }

    if (dirty)
    {
        repaint (nameArea());
        grid.repaint();
    }
    else
    {
        grid.refresh();
    }
}

void TrackRow::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    const auto bounds = getLocalBounds().toFloat();
    const auto colour = t.category (shownCategory);

    if (selected || dropHover)
    {
        const auto r = bounds.reduced (0.0f, 1.0f);
        switch (t.id)
        {
            case ThemeId::PS2Night:
            case ThemeId::Mix:
                g.setGradientFill (juce::ColourGradient (t.accent.withAlpha (dropHover ? 0.25f : 0.14f), r.getX(), 0.0f,
                                                         t.accent.withAlpha (0.0f), r.getRight(), 0.0f, false));
                g.fillRoundedRectangle (r, 6.0f);
                break;
            case ThemeId::Liquid:
                g.setColour (juce::Colours::white.withAlpha (dropHover ? 0.2f : 0.1f));
                g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
                break;
            case ThemeId::Y2KChrome:
                g.setColour (juce::Colours::white.withAlpha (dropHover ? 0.75f : 0.5f));
                g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
                g.setColour (juce::Colours::white);
                g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
                break;
            case ThemeId::Digicore:
            {
                juce::Path border, dashed;
                border.addRectangle (r.reduced (0.5f));
                const float dashes[] = { 3.0f, 3.0f };
                juce::PathStrokeType (1.0f).createDashedStroke (dashed, border, dashes, 2);
                g.setColour (t.accent2.withAlpha (dropHover ? 1.0f : 0.7f));
                g.fillPath (dashed);
                break;
            }
        }
    }

    auto name = nameArea().toFloat();
    auto strip = name.removeFromLeft (6.0f).reduced (0.0f, 2.0f);
    const float glow = 0.35f + 0.65f * flash;
    switch (t.cells)
    {
        case CellStyle::GlassCube:
            g.setColour (colour.withAlpha (0.25f * glow));
            g.fillRoundedRectangle (strip.expanded (2.0f), 3.0f);
            g.setColour (colour.withAlpha (glow));
            g.fillRoundedRectangle (strip, 2.0f);
            break;
        case CellStyle::Gel:
            g.setGradientFill (juce::ColourGradient (colour.brighter (0.6f), strip.getX(), 0.0f, colour.darker (0.1f), strip.getRight(), 0.0f, false));
            g.fillRoundedRectangle (strip, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.4f + 0.6f * flash));
            g.fillRoundedRectangle (strip.reduced (1.5f, 4.0f).withWidth (1.5f), 1.0f);
            break;
        case CellStyle::Pixel:
            g.setColour (colour.withAlpha (glow));
            g.fillRect (strip.toNearestInt());
            break;
    }
    name.removeFromLeft (9.0f);

    const auto label = juce::String (index + 1) + "  " + juce::String (categoryName (shownCategory)).toUpperCase()
                       + (rendering ? "  ..." : "");
    const auto labelArea = name.removeFromTop (name.getHeight() * 0.45f);
    g.setFont (t.font (Theme::Role::Label, 11.0f));
    g.setColour (t.id == ThemeId::Y2KChrome ? colour.darker (0.25f) : colour);
    g.drawText (t.id == ThemeId::Liquid ? label.toUpperCase() : label, labelArea, juce::Justification::bottomLeft);

    g.setColour (t.text.withAlpha (shownName.startsWith ("(") ? 0.45f : 0.95f));
    g.setFont (t.font (Theme::Role::UI, 14.0f));
    g.drawText (shownName, name.translated (0.0f, 1.0f), juce::Justification::topLeft, true);
}

void TrackRow::mouseDown (const juce::MouseEvent& e)
{
    if (nameArea().contains (e.getPosition()))
    {
        if (onSelect)
            onSelect (index);
        processor.auditionTrack (index);   // the track's own pitched + stretched sound
    }
}

//==============================================================================
bool TrackRow::isInterestedInDragSource (const SourceDetails& d)
{
    return d.description.isString();
}

void TrackRow::itemDropped (const SourceDetails& d)
{
    setDropHover (false);
    processor.setTrackSample (index, d.description.toString());
    if (onSelect)
        onSelect (index);
}

bool TrackRow::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isAudioFile (juce::File (f)))
            return true;
    return false;
}

void TrackRow::filesDropped (const juce::StringArray& files, int, int)
{
    setDropHover (false);
    for (const auto& f : files)
        if (isAudioFile (juce::File (f)))
        {
            processor.setTrackSample (index, f);
            break;
        }
}
} // namespace sk
