#include "MapView.h"
#include "Theme.h"

namespace sk
{
MapView::MapView()
{
    visible.fill (true);
    snapshot = std::make_shared<const std::vector<SampleEntry>>();
    setRepaintsOnMouseActivity (false);
}

void MapView::setSnapshot (LibrarySnapshot snap)
{
    snapshot = snap != nullptr ? std::move (snap) : std::make_shared<const std::vector<SampleEntry>>();
    hovered = pressed = -1;
    repaint();
}

void MapView::setCategoryVisible (Category c, bool v)
{
    visible[(size_t) c] = v;
    hovered = -1;
    repaint();
}

void MapView::setHighlightedPath (const juce::String& path)
{
    if (highlightedPath != path)
    {
        highlightedPath = path;
        repaint();
    }
}

void MapView::setStatusText (const juce::String& text)
{
    if (statusText != text)
    {
        statusText = text;
        repaint();
    }
}

//==============================================================================
juce::Rectangle<float> MapView::mapArea() const
{
    auto b = getLocalBounds().toFloat();
    if (SkLookAndFeel::themeOf (*this).id == ThemeId::Digicore)
        b.removeFromTop (18.0f);
    return b.reduced (12.0f);
}

juce::Point<float> MapView::toScreen (float x, float y) const
{
    const auto a = mapArea();
    return { a.getCentreX() + (x - centre.x) * zoom * a.getWidth(),
             a.getCentreY() + (y - centre.y) * zoom * a.getHeight() };
}

juce::Point<float> MapView::fromScreen (juce::Point<float> p) const
{
    const auto a = mapArea();
    return { centre.x + (p.x - a.getCentreX()) / (zoom * a.getWidth()),
             centre.y + (p.y - a.getCentreY()) / (zoom * a.getHeight()) };
}

float MapView::dotRadius() const
{
    return juce::jlimit (2.4f, 7.5f, 3.0f * std::sqrt (zoom));
}

int MapView::hitTestDot (juce::Point<float> p) const
{
    int best = -1;
    float bestDist = 10.0f * 10.0f;
    const auto& entries = *snapshot;

    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! visible[(size_t) e.category])
            continue;
        const auto d = toScreen (e.x, e.y).getDistanceSquaredFrom (p);
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

//==============================================================================
namespace
{
void drawFrame (juce::Graphics& g, juce::Rectangle<float> b, const Theme& t)
{
    switch (t.id)
    {
        case ThemeId::PS2Night:
        case ThemeId::Mix:
        {
            g.setColour (t.panel.withAlpha (0.72f));
            g.fillRoundedRectangle (b, 10.0f);
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path clip;
                clip.addRoundedRectangle (b, 10.0f);
                g.reduceClipRegion (clip);
                skinpaint::towers (g, b.reduced (14.0f, 0.0f), t.id == ThemeId::Mix ? juce::Colour (0xff4f8dff) : t.accent, 5);

                juce::Random rng (3);
                for (int i = 0; i < 90; ++i)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.12f + 0.35f * rng.nextFloat()));
                    const float s = rng.nextFloat() < 0.15f ? 2.0f : 1.0f;
                    g.fillRect (b.getX() + rng.nextFloat() * b.getWidth(), b.getY() + rng.nextFloat() * b.getHeight() * 0.7f, s, s);
                }
            }
            if (t.id == ThemeId::Mix)
            {
                g.setGradientFill (juce::ColourGradient (t.accent.withAlpha (0.55f), b.getX(), 0.0f, t.accent2.withAlpha (0.55f), b.getRight(), 0.0f, false));
                g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.2f);
            }
            else
            {
                g.setColour (t.panelEdge.withAlpha (0.45f));
                g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
            }
            break;
        }

        case ThemeId::Liquid:
        {
            const float rad = 18.0f;
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.13f), 0.0f, b.getY(),
                                                     juce::Colours::white.withAlpha (0.04f), 0.0f, b.getBottom(), false));
            g.fillRoundedRectangle (b, rad);
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path clip;
                clip.addRoundedRectangle (b, rad);
                g.reduceClipRegion (clip);
                skinpaint::lightStreak (g, b, 0.7f);
            }
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.2f);
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.fillRect (b.reduced (rad, 0.0f).withHeight (1.0f).translated (0.0f, 2.0f));
            break;
        }

        case ThemeId::Y2KChrome:
        {
            const float rad = 18.0f;
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.fillRoundedRectangle (b, rad);
            {
                juce::Graphics::ScopedSaveState ss (g);
                juce::Path clip;
                clip.addRoundedRectangle (b, rad);
                g.reduceClipRegion (clip);
                g.setColour (juce::Colour (0xff9aa7c0).withAlpha (0.16f));
                for (float x = b.getX(); x < b.getRight(); x += 28.0f) g.fillRect (x, b.getY(), 1.0f, b.getHeight());
                for (float y = b.getY(); y < b.getBottom(); y += 28.0f) g.fillRect (b.getX(), y, b.getWidth(), 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.drawEllipse (juce::Rectangle<float> (b.getWidth() * 0.9f, b.getHeight() * 0.55f).withCentre (b.getCentre()), 1.2f);
            }
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (b.reduced (0.75f), rad, 1.5f);
            g.setColour (juce::Colour (0xff9aa7c0).withAlpha (0.6f));
            g.drawRoundedRectangle (b.reduced (2.5f), rad - 2.0f, 1.0f);
            break;
        }

        case ThemeId::Digicore:
        {
            auto body = b.withTrimmedRight (3.0f).withTrimmedBottom (3.0f);
            g.setColour (t.accent);
            g.drawRect (body.translated (3.0f, 3.0f), 1.0f);
            g.setColour (t.panel.withAlpha (0.95f));
            g.fillRect (body);

            g.setColour (t.accent2.withAlpha (0.06f));
            for (float x = body.getX(); x < body.getRight(); x += 16.0f) g.fillRect (x, body.getY(), 1.0f, body.getHeight());
            for (float y = body.getY(); y < body.getBottom(); y += 16.0f) g.fillRect (body.getX(), y, body.getWidth(), 1.0f);

            // old-web window title bar
            auto bar = body.removeFromTop (18.0f);
            g.setGradientFill (juce::ColourGradient (t.accent, bar.getX(), 0.0f, juce::Colour (0xff6a00ff), bar.getRight(), 0.0f, false));
            g.fillRect (bar);
            g.setColour (juce::Colours::white);
            g.setFont (t.font (Theme::Role::Label, 11.0f));
            g.drawText ("sample_map.exe", bar.reduced (6.0f, 0.0f), juce::Justification::centredLeft);
            for (int i = 0; i < 3; ++i)
            {
                const auto box = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ bar.getRight() - 12.0f - (float) i * 16.0f, bar.getCentreY() });
                g.setColour (juce::Colour (0xff120a1f));
                g.fillRect (box);
                g.setColour (juce::Colours::white);
                g.drawRect (box, 1.0f);
            }
            g.setColour (t.accent2);
            g.drawRect (b.withTrimmedRight (3.0f).withTrimmedBottom (3.0f), 1.0f);
            break;
        }
    }
}

void drawDot (juce::Graphics& g, juce::Point<float> p, float r, juce::Colour col, const Theme& t, bool halo)
{
    switch (t.dots)
    {
        case DotStyle::Glow:
            if (halo)
            {
                g.setGradientFill (juce::ColourGradient (col.withAlpha (0.35f), p, col.withAlpha (0.0f), p.translated (r * 3.2f, 0.0f), true));
                g.fillEllipse (juce::Rectangle<float> (r * 6.4f, r * 6.4f).withCentre (p));
            }
            g.setColour (col);
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (p));
            g.setColour (juce::Colours::white.withAlpha (0.65f));
            g.fillEllipse (juce::Rectangle<float> (r * 0.8f, r * 0.8f).withCentre (p));
            break;

        case DotStyle::Bubble:
        {
            const auto e = juce::Rectangle<float> (r * 2.4f, r * 2.4f).withCentre (p);
            g.setGradientFill (juce::ColourGradient (col.brighter (0.7f), e.getX(), e.getY(), col.darker (0.15f), e.getRight(), e.getBottom(), false));
            g.fillEllipse (e);
            g.setColour (col.darker (0.4f).withAlpha (0.6f));
            g.drawEllipse (e, 0.7f);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillEllipse (e.getX() + e.getWidth() * 0.22f, e.getY() + e.getHeight() * 0.16f, e.getWidth() * 0.32f, e.getHeight() * 0.24f);
            break;
        }

        case DotStyle::ChromeBead:
        {
            const auto e = juce::Rectangle<float> (r * 2.5f, r * 2.5f).withCentre (p);
            g.setColour (juce::Colour (0xff10104a).withAlpha (0.35f));
            g.fillEllipse (e.translated (0.8f, 1.4f));
            juce::ColourGradient grad (juce::Colours::white, e.getCentreX(), e.getY(), col.darker (0.2f), e.getCentreX(), e.getBottom(), false);
            grad.addColour (0.40, col.brighter (0.6f));
            grad.addColour (0.56, col.darker (0.55f));
            grad.addColour (0.75, col);
            g.setGradientFill (grad);
            g.fillEllipse (e);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (e.getX() + e.getWidth() * 0.26f, e.getY() + e.getHeight() * 0.12f, e.getWidth() * 0.3f, e.getHeight() * 0.2f);
            break;
        }

        case DotStyle::Square:
        {
            const auto s = std::round (r * 2.0f);
            g.setColour (col);
            g.fillRect (juce::Rectangle<float> (s, s).withCentre (p).toNearestInt().toFloat());
            break;
        }
    }
}
} // namespace

void MapView::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    const auto bounds = getLocalBounds().toFloat();
    drawFrame (g, bounds, t);

    g.saveState();
    g.reduceClipRegion (mapArea().expanded (10.0f).getIntersection (bounds).toNearestInt());

    const auto& entries = *snapshot;

    // island labels
    for (int c = 0; c < kNumCategories; ++c)
    {
        if (! visible[(size_t) c])
            continue;
        const auto cat = (Category) c;
        const auto ctr = categoryCentre (cat);
        const auto p = toScreen (ctr.x, ctr.y - 0.16f);
        const auto area = juce::Rectangle<float> (160.0f, 20.0f).withCentre (p);
        const auto col = t.category (cat);
        const auto font = t.font (Theme::Role::Label, 13.0f + 2.0f * std::sqrt (zoom));
        const auto name = juce::String (categoryName (cat)).toUpperCase();

        if (t.glitch && t.id == ThemeId::Digicore)
            skinpaint::glitchText (g, name, font, area, juce::Justification::centred, col, 1.0f);
        else
        {
            g.setFont (font);
            g.setColour (t.id == ThemeId::Y2KChrome ? col.darker (0.25f).withAlpha (0.8f)
                         : t.id == ThemeId::Liquid ? juce::Colours::white.withAlpha (0.75f) : col.withAlpha (0.6f));
            g.drawText (name, area, juce::Justification::centred);
        }
    }

    // dots
    const float r = dotRadius();
    const bool halos = entries.size() < 3000;
    int highlighted = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! visible[(size_t) e.category])
            continue;

        const auto p = toScreen (e.x, e.y);
        if (! bounds.expanded (r).contains (p))
            continue;

        drawDot (g, p, r, t.category (e.category), t, halos);

        if (highlightedPath.isNotEmpty() && e.path == highlightedPath)
            highlighted = i;
    }

    auto ring = [&] (int idx, float extra, juce::Colour colour)
    {
        const auto& e = entries[(size_t) idx];
        const auto p = toScreen (e.x, e.y);
        const float rr = r + extra;
        g.setColour (colour);
        if (t.dots == DotStyle::Square)
            g.drawRect (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (p).toNearestInt().toFloat(), 1.5f);
        else
            g.drawEllipse (p.x - rr, p.y - rr, rr * 2.0f, rr * 2.0f, 1.6f);
    };

    const auto ringColour = t.id == ThemeId::Y2KChrome ? t.text : juce::Colours::white;

    if (highlighted >= 0)
    {
        ring (highlighted, 3.5f, ringColour.withAlpha (0.85f));
        if (t.sparkles)
        {
            const auto& e = entries[(size_t) highlighted];
            skinpaint::sparkle (g, toScreen (e.x, e.y).translated (r + 7.0f, -r - 7.0f), 6.0f,
                                t.id == ThemeId::Y2KChrome ? t.accent : juce::Colours::white);
        }
    }

    if (hovered >= 0 && hovered < (int) entries.size())
    {
        const auto& e = entries[(size_t) hovered];
        const auto p = toScreen (e.x, e.y);
        ring (hovered, 2.0f, ringColour);

        const auto font = t.font (Theme::Role::UIBold, 14.0f);
        g.setFont (font);
        const auto text = e.name + "   " + juce::String (e.duration, 2) + " s";
        const float w = juce::jmin (380.0f, (float) juce::GlyphArrangement::getStringWidthInt (font, text) + 18.0f);
        auto box = juce::Rectangle<float> (w, 24.0f).withPosition (p.x + 10.0f, p.y - 32.0f);
        box = box.constrainedWithin (bounds.reduced (4.0f));

        switch (t.id)
        {
            case ThemeId::Y2KChrome:
                g.setColour (juce::Colours::white.withAlpha (0.95f));
                g.fillRoundedRectangle (box, box.getHeight() * 0.5f);
                g.setColour (juce::Colour (0xff9aa7c0));
                g.drawRoundedRectangle (box, box.getHeight() * 0.5f, 1.0f);
                break;
            case ThemeId::Digicore:
                g.setColour (juce::Colour (0xff120a1f));
                g.fillRect (box);
                g.setColour (t.accent2);
                g.drawRect (box, 1.0f);
                break;
            case ThemeId::Liquid:
                g.setColour (juce::Colour (0xee1d1e6e));
                g.fillRoundedRectangle (box, box.getHeight() * 0.5f);
                g.setColour (juce::Colours::white.withAlpha (0.5f));
                g.drawRoundedRectangle (box, box.getHeight() * 0.5f, 1.0f);
                break;
            case ThemeId::PS2Night:
            case ThemeId::Mix:
                g.setColour (juce::Colour (0xee050914));
                g.fillRoundedRectangle (box, 5.0f);
                g.setColour (t.accent.withAlpha (0.6f));
                g.drawRoundedRectangle (box, 5.0f, 1.0f);
                break;
        }
        g.setColour (t.text);
        g.drawText (text, box.reduced (9.0f, 0.0f), juce::Justification::centredLeft, true);
    }

    if (entries.empty())
    {
        g.setColour (t.textDim);
        g.setFont (t.font (Theme::Role::UI, 17.0f));
        g.drawText ("Click \"Add Folder\" and choose your drum sample folder to build the map",
                    mapArea(), juce::Justification::centred);
    }

    g.restoreState();

    if (statusText.isNotEmpty())
    {
        g.setColour (t.textDim);
        g.setFont (t.font (Theme::Role::Label, 11.0f));
        g.drawText (statusText, bounds.reduced (14.0f, 8.0f), juce::Justification::bottomLeft);
    }
}

//==============================================================================
void MapView::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTestDot (e.position);
    if (h != hovered)
    {
        hovered = h;
        if (e.mods.isAltDown() && hovered >= 0 && onPreview)
            onPreview ((*snapshot)[(size_t) hovered].path);
        repaint();
    }
}

void MapView::mouseExit (const juce::MouseEvent&)
{
    if (hovered >= 0)
    {
        hovered = -1;
        repaint();
    }
}

void MapView::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = false;
    pressed = hitTestDot (e.position);

    if (pressed >= 0)
    {
        const auto& path = (*snapshot)[(size_t) pressed].path;
        setHighlightedPath (path);
        if (onPreview)
            onPreview (path);
        panning = false;
    }
    else
    {
        panning = true;
        panStartCentre = centre;
    }
}

void MapView::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed >= 0)
    {
        if (! dragStarted && e.getDistanceFromDragStart() > 6)
        {
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                const auto& entry = (*snapshot)[(size_t) pressed];
                const auto& t = SkLookAndFeel::themeOf (*this);
                juce::Image img (juce::Image::ARGB, 20, 20, true);
                {
                    juce::Graphics ig (img);
                    drawDot (ig, { 10.0f, 10.0f }, 6.0f, t.category (entry.category), t, false);
                }
                container->startDragging (juce::var (entry.path), this, juce::ScaledImage (img));
                dragStarted = true;
            }
        }
        return;
    }

    if (panning)
    {
        const auto a = mapArea();
        const auto d = e.position - e.mouseDownPosition;
        centre = { panStartCentre.x - d.x / (zoom * a.getWidth()),
                   panStartCentre.y - d.y / (zoom * a.getHeight()) };
        repaint();
    }
}

void MapView::mouseUp (const juce::MouseEvent&)
{
    pressed = -1;
    panning = false;
    dragStarted = false;
}

void MapView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hitTestDot (e.position);
    if (h >= 0 && onAssign)
        onAssign ((*snapshot)[(size_t) h].path);
    else if (h < 0)
    {
        zoom = 1.0f;
        centre = { 0.5f, 0.5f };
        repaint();
    }
}

void MapView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto before = fromScreen (e.position);
    zoom = juce::jlimit (1.0f, 16.0f, zoom * std::exp (w.deltaY * 1.6f));
    const auto after = fromScreen (e.position);
    centre += before - after;
    if (zoom <= 1.0001f)
        centre = { 0.5f, 0.5f };
    repaint();
}
} // namespace sk
