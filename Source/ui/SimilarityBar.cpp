#include "SimilarityBar.h"
#include "Theme.h"

namespace sk
{
SimilarityBar::SimilarityBar()
{
    setTooltip ("Similar sounds. Click a dot to listen, drag it onto a track to load it. "
                "> plays them in turn, History goes back to earlier picks.");
}

void SimilarityBar::setSeed (LibrarySnapshot snap, int index, bool addToHistory)
{
    snapshot = std::move (snap);
    if (snapshot == nullptr || ! juce::isPositiveAndBelow (index, (int) snapshot->size()))
    {
        clear();
        return;
    }

    seed = index;
    current = index;
    currentSlot = 0;
    neighbours = SampleLibrary::nearest (*snapshot, index, kNeighbours);

    if (addToHistory)
    {
        const auto& path = (*snapshot)[(size_t) index].path;
        if (historyPos >= 0 && historyPos < (int) history.size() - 1)
            history.resize ((size_t) historyPos + 1);
        if (history.empty() || history.back() != path)
            history.push_back (path);
        if (history.size() > 200)
            history.erase (history.begin());
        historyPos = (int) history.size() - 1;
    }

    if (onSelectionChanged)
        onSelectionChanged();
    repaint();
}

void SimilarityBar::clear()
{
    seed = current = -1;
    neighbours.clear();
    playingThrough = false;
    stopTimer();
    if (onSelectionChanged)
        onSelectionChanged();
    repaint();
}

//==============================================================================
void SimilarityBar::layout (juce::Rectangle<float>& first, juce::Rectangle<float>& prev, juce::Rectangle<float>& play,
                            juce::Rectangle<float>& dots, juce::Rectangle<float>& hist, juce::Rectangle<float>& up,
                            juce::Rectangle<float>& down, juce::Rectangle<float>& heart, juce::Rectangle<float>& load) const
{
    auto r = getLocalBounds().toFloat().reduced (14.0f, 0.0f);
    const float h = r.getHeight();
    r.removeFromLeft (84.0f);                         // "Similarity" label
    first = r.removeFromLeft (24.0f);
    prev  = r.removeFromLeft (24.0f);
    play  = r.removeFromLeft (26.0f);
    r.removeFromLeft (8.0f);

    load  = r.removeFromRight (34.0f);
    r.removeFromRight (6.0f);
    heart = r.removeFromRight (30.0f);
    r.removeFromRight (10.0f);
    auto arrows = r.removeFromRight (20.0f);
    up   = arrows.withHeight (h * 0.5f);
    down = arrows.withTrimmedTop (h * 0.5f);
    hist = r.removeFromRight (66.0f);
    r.removeFromRight (10.0f);
    dots = r;
}

juce::Point<float> SimilarityBar::slotCentre (int slot) const
{
    juce::Rectangle<float> a, b, c, dots, d, e, f, g, h;
    layout (a, b, c, dots, d, e, f, g, h);
    const int slots = kNeighbours + 1;
    const float first = dots.getX() + 18.0f;
    const float step = (dots.getWidth() - 26.0f) / (float) (slots - 1);
    return { first + step * (float) slot, dots.getCentreY() };
}

int SimilarityBar::indexForSlot (int slot) const
{
    if (slot == 0) return seed;
    if (juce::isPositiveAndBelow (slot - 1, (int) neighbours.size())) return neighbours[(size_t) slot - 1];
    return -1;
}

SimilarityBar::Hit SimilarityBar::hitAt (juce::Point<float> p) const
{
    juce::Rectangle<float> first, prev, play, dots, hist, up, down, heart, load;
    layout (first, prev, play, dots, hist, up, down, heart, load);
    if (first.contains (p)) return { Part::first };
    if (prev.contains (p))  return { Part::prev };
    if (play.contains (p))  return { Part::play };
    if (up.contains (p))    return { Part::histUp };
    if (down.contains (p))  return { Part::histDown };
    if (hist.contains (p))  return { Part::history };
    if (heart.contains (p)) return { Part::heart };
    if (load.contains (p))  return { Part::load };
    if (dots.contains (p) && seed >= 0)
    {
        int best = -1;
        float bestD = 14.0f;
        for (int s = 0; s <= (int) neighbours.size(); ++s)
        {
            const float d = std::abs (slotCentre (s).x - p.x);
            if (d < bestD) { bestD = d; best = s; }
        }
        if (best >= 0)
            return { Part::dot, best };
    }
    return {};
}

//==============================================================================
void SimilarityBar::playSlot (int slot)
{
    const int idx = indexForSlot (slot);
    if (idx < 0)
        return;
    currentSlot = slot;
    current = idx;
    if (onAudition)
        onAudition (idx);
    if (onSelectionChanged)
        onSelectionChanged();
    repaint();
}

void SimilarityBar::step (int delta)
{
    if (seed < 0)
        return;
    const int last = (int) neighbours.size();
    playSlot (juce::jlimit (0, last, currentSlot + delta));
}

void SimilarityBar::goHistory (int delta)
{
    if (snapshot == nullptr || history.empty())
        return;
    const int target = juce::jlimit (0, (int) history.size() - 1, historyPos + delta);
    if (target == historyPos && seed >= 0)
        return;
    const auto& path = history[(size_t) target];
    for (int i = 0; i < (int) snapshot->size(); ++i)
        if ((*snapshot)[(size_t) i].path == path)
        {
            historyPos = target;
            setSeed (snapshot, i, false);
            if (onSeedChanged) onSeedChanged (i);
            if (onAudition) onAudition (i);
            return;
        }
}

void SimilarityBar::timerCallback()
{
    if (currentSlot >= (int) neighbours.size())
    {
        playingThrough = false;
        stopTimer();
        repaint();
        return;
    }
    step (1);
}

//==============================================================================
void SimilarityBar::mouseMove (const juce::MouseEvent& e)
{
    const auto h = hitAt (e.position);
    if (h.part != hover.part || h.slot != hover.slot)
    {
        hover = h;
        repaint();
    }
}

void SimilarityBar::mouseExit (const juce::MouseEvent&)
{
    hover = {};
    repaint();
}

void SimilarityBar::mouseDown (const juce::MouseEvent& e)
{
    pressed = hitAt (e.position);
    switch (pressed.part)
    {
        case Part::first:   playingThrough = false; stopTimer(); playSlot (0); break;
        case Part::prev:    playingThrough = false; stopTimer(); step (-1); break;
        case Part::play:
            playingThrough = ! playingThrough;
            if (playingThrough)
            {
                if (currentSlot >= (int) neighbours.size())
                    currentSlot = -1;
                step (1);
                startTimer (380);
            }
            else
                stopTimer();
            repaint();
            break;
        case Part::dot:     playingThrough = false; stopTimer(); playSlot (pressed.slot); break;
        case Part::histUp:
        case Part::history: goHistory (-1); break;
        case Part::histDown: goHistory (1); break;
        case Part::heart:
            if (snapshot != nullptr && getCurrent() >= 0 && setFavourite && isFavourite)
            {
                const auto& path = (*snapshot)[(size_t) getCurrent()].path;
                setFavourite (path, ! isFavourite (path));
                repaint();
            }
            break;
        case Part::load:
            if (snapshot != nullptr && getCurrent() >= 0 && onLoad)
                onLoad ((*snapshot)[(size_t) getCurrent()].path);
            break;
        case Part::none:
        default: break;
    }
}

void SimilarityBar::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed.part != Part::dot || e.getDistanceFromDragStart() < 6 || snapshot == nullptr)
        return;
    const int idx = indexForSlot (pressed.slot);
    if (idx < 0)
        return;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
    {
        const auto& t = SkLookAndFeel::themeOf (*this);
        const auto& entry = (*snapshot)[(size_t) idx];
        juce::Image img (juce::Image::ARGB, 20, 20, true);
        {
            juce::Graphics ig (img);
            ig.setColour (t.category (entry.category));
            ig.fillEllipse (3.0f, 3.0f, 14.0f, 14.0f);
            ig.setColour (juce::Colours::white);
            ig.drawEllipse (3.0f, 3.0f, 14.0f, 14.0f, 1.5f);
        }
        pressed = {};
        container->startDragging (juce::var (entry.path), this, juce::ScaledImage (img));
    }
}

void SimilarityBar::mouseUp (const juce::MouseEvent&)
{
    pressed = {};
}

//==============================================================================
void SimilarityBar::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    const bool light = t.id == ThemeId::Y2KChrome;
    const auto ink = light ? t.text : juce::Colours::white;

    // pill
    switch (t.buttons)
    {
        case ButtonStyle::Pixel:
            g.setColour (t.panel.withAlpha (0.95f)); g.fillRect (b);
            g.setColour (t.accent2.withAlpha (0.7f)); g.drawRect (b, 1.0f);
            break;
        case ButtonStyle::Glass:
        case ButtonStyle::Chrome:
        default:
            g.setColour (light ? juce::Colours::white.withAlpha (0.6f)
                               : t.id == ThemeId::Liquid ? juce::Colours::white.withAlpha (0.08f)
                                                         : juce::Colour (0xff070d24).withAlpha (0.75f));
            g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
            g.setColour (light ? juce::Colours::white : ink.withAlpha (0.28f));
            g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.2f);
            break;
    }

    juce::Rectangle<float> first, prev, play, dots, hist, up, down, heart, load;
    layout (first, prev, play, dots, hist, up, down, heart, load);

    auto inkFor = [&] (Part p) { return ink.withAlpha (hover.part == p ? 1.0f : (seed >= 0 ? 0.72f : 0.3f)); };

    g.setFont (t.font (Theme::Role::UI, 15.0f));
    g.setColour (ink.withAlpha (0.75f));
    g.drawText ("Similarity", getLocalBounds().toFloat().reduced (16.0f, 0.0f).withWidth (84.0f),
                juce::Justification::centredLeft);

    auto triangle = [] (juce::Rectangle<float> r, bool right)
    {
        juce::Path p;
        const auto c = r.getCentre();
        const float s = 6.0f;
        if (right) p.addTriangle (c.x - s * 0.7f, c.y - s, c.x - s * 0.7f, c.y + s, c.x + s * 0.9f, c.y);
        else       p.addTriangle (c.x + s * 0.7f, c.y - s, c.x + s * 0.7f, c.y + s, c.x - s * 0.9f, c.y);
        return p;
    };

    g.setColour (inkFor (Part::first));
    g.fillPath (triangle (first.translated (2.0f, 0.0f), false));
    g.fillRect (juce::Rectangle<float> (2.0f, 12.0f).withCentre (first.getCentre().translated (-5.0f, 0.0f)));
    g.setColour (inkFor (Part::prev));
    g.fillPath (triangle (prev, false));
    g.setColour (inkFor (Part::play));
    if (playingThrough)
    {
        g.fillRect (juce::Rectangle<float> (3.0f, 12.0f).withCentre (play.getCentre().translated (-3.5f, 0.0f)));
        g.fillRect (juce::Rectangle<float> (3.0f, 12.0f).withCentre (play.getCentre().translated (3.5f, 0.0f)));
    }
    else
        g.fillPath (triangle (play, true));

    // dots
    if (seed >= 0 && snapshot != nullptr)
    {
        for (int s = (int) neighbours.size(); s >= 0; --s)
        {
            const int idx = indexForSlot (s);
            if (idx < 0)
                continue;
            const auto& e = (*snapshot)[(size_t) idx];
            const auto c = slotCentre (s);
            auto col = t.category (e.category);
            if (light) col = col.darker (0.15f);
            const float fade = s == 0 ? 1.0f : juce::jmap ((float) s, 1.0f, (float) kNeighbours, 1.0f, 0.45f);
            const float r = s == 0 ? 8.0f : (hover.part == Part::dot && hover.slot == s ? 5.5f : 4.0f);
            g.setColour (col.withMultipliedAlpha (fade));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
            if (s == 0)
            {
                g.setColour (ink);
                g.drawEllipse (juce::Rectangle<float> (30.0f, 30.0f).withCentre (c), 1.8f);
            }
            if (s == currentSlot && s != 0)
            {
                g.setColour (ink);
                g.drawEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (c), 1.4f);
            }
        }
    }
    else
    {
        g.setColour (ink.withAlpha (0.45f));
        g.setFont (t.font (Theme::Role::UI, 14.0f));
        g.drawText ("Click a sound on the map to see similar sounds here", dots, juce::Justification::centred);
    }

    // history
    g.setColour (inkFor (Part::history));
    g.setFont (t.font (Theme::Role::UI, 15.0f));
    g.drawText ("History", hist, juce::Justification::centredRight);
    g.setColour (inkFor (Part::histUp));
    {
        juce::Path p; const auto c = up.getCentre().translated (0.0f, 2.0f);
        p.addTriangle (c.x - 6.0f, c.y + 3.0f, c.x + 6.0f, c.y + 3.0f, c.x, c.y - 5.0f);
        g.fillPath (p);
    }
    g.setColour (inkFor (Part::histDown));
    {
        juce::Path p; const auto c = down.getCentre().translated (0.0f, -2.0f);
        p.addTriangle (c.x - 6.0f, c.y - 3.0f, c.x + 6.0f, c.y - 3.0f, c.x, c.y + 5.0f);
        g.fillPath (p);
    }

    // heart
    {
        const bool fav = snapshot != nullptr && getCurrent() >= 0 && isFavourite
                         && isFavourite ((*snapshot)[(size_t) getCurrent()].path);
        const auto c = heart.getCentre();
        juce::Path p;
        const float s = 8.0f;
        p.startNewSubPath (c.x, c.y + s);
        p.cubicTo (c.x - s * 1.6f, c.y - s * 0.1f, c.x - s * 0.9f, c.y - s * 1.3f, c.x, c.y - s * 0.45f);
        p.cubicTo (c.x + s * 0.9f, c.y - s * 1.3f, c.x + s * 1.6f, c.y - s * 0.1f, c.x, c.y + s);
        p.closeSubPath();
        if (fav)
        {
            g.setColour (light ? t.accent : juce::Colour (0xffff5fb8));
            g.fillPath (p);
        }
        g.setColour (fav ? (light ? t.accent : juce::Colour (0xffff5fb8)) : inkFor (Part::heart));
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

    // load into selected track
    {
        const auto c = load.getCentre();
        g.setColour (inkFor (Part::load));
        g.drawRoundedRectangle (juce::Rectangle<float> (20.0f, 13.0f).withCentre (c.translated (0.0f, 2.0f)), 2.5f, 1.5f);
        juce::Path arrow;
        arrow.addArrow ({ c.x, c.y - 9.0f, c.x, c.y + 2.0f }, 1.6f, 7.0f, 5.0f);
        g.fillPath (arrow);
    }
}
} // namespace sk
