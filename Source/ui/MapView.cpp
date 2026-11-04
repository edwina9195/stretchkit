#include "MapView.h"
#include "Theme.h"

#include <cmath>

namespace sk
{
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


//==============================================================================
MapView::MapView()
{
    visible.fill (true);
    snapshot = std::make_shared<const std::vector<SampleEntry>>();
    setRepaintsOnMouseActivity (false);
}

void MapView::setSnapshot (LibrarySnapshot snap)
{
    snapshot = snap != nullptr ? std::move (snap) : std::make_shared<const std::vector<SampleEntry>>();
    hovered = lastScrub = -1;
    if (seed >= (int) snapshot->size()) seed = -1;
    if (playing >= (int) snapshot->size()) playing = -1;
    neighbours.erase (std::remove_if (neighbours.begin(), neighbours.end(),
                                      [this] (int i) { return i >= (int) snapshot->size(); }), neighbours.end());
    dirty = true;
    repaint();
}

void MapView::setCategoryVisible (Category c, bool v)
{
    visible[(size_t) c] = v;
    hovered = -1;
    dirty = true;
    repaint();
}

void MapView::setFilter (const juce::String& query, bool favOnly, std::function<bool (const juce::String&)> isFav)
{
    searchWords = juce::StringArray::fromTokens (query.toLowerCase(), " ", "");
    searchWords.removeEmptyStrings();
    favouritesOnly = favOnly;
    isFavourite = std::move (isFav);
    hovered = -1;
    dirty = true;
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

void MapView::setSelection (int seedIndex, const std::vector<int>& nb, int playingIndex)
{
    seed = seedIndex;
    neighbours = nb;
    playing = playingIndex;
    repaint();
}

void MapView::setStatusText (const juce::String& text)
{
    if (statusText != text)
    {
        statusText = text;
        repaint();
    }
}

bool MapView::passesFilter (const SampleEntry& e) const
{
    if (! visible[(size_t) e.category])
        return false;
    if (favouritesOnly && (isFavourite == nullptr || ! isFavourite (e.path)))
        return false;
    if (! searchWords.isEmpty())
    {
        const auto name = e.name.toLowerCase();
        for (const auto& w : searchWords)
            if (! name.contains (w))
                return false;
    }
    return true;
}

//==============================================================================
juce::Rectangle<float> MapView::mapArea() const
{
    auto b = getLocalBounds().toFloat();
    if (SkLookAndFeel::themeOf (*this).id == ThemeId::Digicore)
        b.removeFromTop (18.0f);
    return b.reduced (14.0f, 12.0f);
}

float MapView::pixelsPerUnit() const
{
    const auto a = mapArea();
    return juce::jmin (a.getWidth() / kWorldW, a.getHeight()) * zoom;
}

juce::Point<float> MapView::toScreen (float x, float y) const
{
    const auto a = mapArea();
    const float s = pixelsPerUnit();
    return { a.getCentreX() + (x - centre.x) * s, a.getCentreY() + (y - centre.y) * s };
}

juce::Point<float> MapView::toWorld (juce::Point<float> p) const
{
    const auto a = mapArea();
    const float s = pixelsPerUnit();
    return { centre.x + (p.x - a.getCentreX()) / s, centre.y + (p.y - a.getCentreY()) / s };
}

void MapView::clampView()
{
    zoom = juce::jlimit (1.0f, 40.0f, zoom);
    centre.x = juce::jlimit (0.0f, kWorldW, centre.x);
    centre.y = juce::jlimit (0.0f, 1.0f, centre.y);
    if (zoom <= 1.0001f)
        centre = { kWorldW * 0.5f, 0.5f };
}

void MapView::zoomAround (juce::Point<float> anchor, float factor)
{
    const auto before = toWorld (anchor);
    zoom *= factor;
    clampView();
    const auto after = toWorld (anchor);
    centre += before - after;
    clampView();
    dirty = true;
    repaint();
}

float MapView::dotRadius() const
{
    const int c = juce::jmax (1, visibleCount);
    const float base = c < 400 ? 3.4f : c < 3000 ? 2.5f : c < 20000 ? 1.8f : c < 60000 ? 1.35f : 1.1f;
    return juce::jlimit (1.0f, 6.0f, base * std::pow (zoom, 0.45f));
}

//==============================================================================
void MapView::rebuildCache()
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    const auto& entries = *snapshot;
    const auto area = mapArea();
    cacheBounds = area.expanded (8.0f).getIntersection (getLocalBounds().toFloat()).getSmallestIntegerContainer();
    cacheTheme = (int) t.id;
    dirty = false;

    // 1) which dots are visible, and where
    visibleIdx.clear();
    screenPos.clear();
    int passing = 0;
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! passesFilter (e))
            continue;
        ++passing;
        const auto p = toScreen (e.x, e.y);
        if (cacheBounds.toFloat().contains (p))
        {
            visibleIdx.push_back (i);
            screenPos.push_back (p);
        }
    }
    visibleCount = passing;

    // 2) hit-test grid
    gridW = juce::jmax (1, cacheBounds.getWidth() / kCell + 1);
    gridH = juce::jmax (1, cacheBounds.getHeight() / kCell + 1);
    grid.assign ((size_t) (gridW * gridH), {});
    for (int k = 0; k < (int) screenPos.size(); ++k)
    {
        const int gx = juce::jlimit (0, gridW - 1, (int) ((screenPos[(size_t) k].x - (float) cacheBounds.getX()) / kCell));
        const int gy = juce::jlimit (0, gridH - 1, (int) ((screenPos[(size_t) k].y - (float) cacheBounds.getY()) / kCell));
        grid[(size_t) (gy * gridW + gx)].push_back (k);
    }

    // 3) plot the dots straight into pixels
    const float scale = juce::jlimit (1.0f, 3.0f, (float) juce::Component::getApproximateScaleFactorForComponent (this));
    const int iw = juce::jmax (1, (int) std::ceil ((float) cacheBounds.getWidth() * scale));
    const int ih = juce::jmax (1, (int) std::ceil ((float) cacheBounds.getHeight() * scale));
    if (! cache.isValid() || cache.getWidth() != iw || cache.getHeight() != ih)
        cache = juce::Image (juce::Image::ARGB, iw, ih, true);
    else
        cache.clear (cache.getBounds());

    const float r = dotRadius() * scale;
    const bool square = t.dots == DotStyle::Square;
    struct Tap { int dx, dy; juce::uint8 cover; };
    std::vector<Tap> kernel;
    const int reach = (int) std::ceil (r) + 1;
    for (int dy = -reach; dy <= reach; ++dy)
        for (int dx = -reach; dx <= reach; ++dx)
        {
            float cover;
            if (square)
                cover = (std::abs ((float) dx) <= r - 0.5f && std::abs ((float) dy) <= r - 0.5f) ? 1.0f : 0.0f;
            else
            {
                int inside = 0;   // 4x4 supersampled coverage
                for (int sy = 0; sy < 4; ++sy)
                    for (int sx = 0; sx < 4; ++sx)
                    {
                        const float fx = (float) dx + ((float) sx + 0.5f) / 4.0f - 0.5f;
                        const float fy = (float) dy + ((float) sy + 0.5f) / 4.0f - 0.5f;
                        inside += (fx * fx + fy * fy <= r * r) ? 1 : 0;
                    }
                cover = (float) inside / 16.0f;
            }
            if (cover > 0.0f)
                kernel.push_back ({ dx, dy, (juce::uint8) juce::jlimit (1, 255, (int) (cover * 255.0f)) });
        }

    const bool light = t.id == ThemeId::Y2KChrome;
    const float alpha = visibleCount > 40000 ? 0.85f : 0.95f;
    std::array<juce::PixelARGB, kNumCategories> colours;
    for (int c = 0; c < kNumCategories; ++c)
    {
        auto col = t.category ((Category) c);
        if (light) col = col.darker (0.15f);
        else col = col.brighter (t.id == ThemeId::Liquid ? 0.15f : 0.25f);
        auto px = col.withAlpha (alpha).getPixelARGB();
        px.premultiply();
        colours[(size_t) c] = px;
    }

    juce::Image::BitmapData bd (cache, juce::Image::BitmapData::readWrite);
    for (int k = 0; k < (int) visibleIdx.size(); ++k)
    {
        const auto& p = screenPos[(size_t) k];
        const int cx = (int) std::lround ((p.x - (float) cacheBounds.getX()) * scale);
        const int cy = (int) std::lround ((p.y - (float) cacheBounds.getY()) * scale);
        const auto src = colours[(size_t) entries[(size_t) visibleIdx[(size_t) k]].category];
        for (const auto& tap : kernel)
        {
            const int x = cx + tap.dx, y = cy + tap.dy;
            if (x < 0 || y < 0 || x >= iw || y >= ih)
                continue;
            reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y))->blend (src, tap.cover);
        }
    }
}

int MapView::findDot (juce::Point<float> p, float radius) const
{
    if (grid.empty())
        return -1;
    int best = -1;
    float bestD = radius * radius;
    const int gx0 = (int) std::floor ((p.x - radius - (float) cacheBounds.getX()) / kCell);
    const int gx1 = (int) std::floor ((p.x + radius - (float) cacheBounds.getX()) / kCell);
    const int gy0 = (int) std::floor ((p.y - radius - (float) cacheBounds.getY()) / kCell);
    const int gy1 = (int) std::floor ((p.y + radius - (float) cacheBounds.getY()) / kCell);
    for (int gy = juce::jmax (0, gy0); gy <= juce::jmin (gridH - 1, gy1); ++gy)
        for (int gx = juce::jmax (0, gx0); gx <= juce::jmin (gridW - 1, gx1); ++gx)
            for (int k : grid[(size_t) (gy * gridW + gx)])
            {
                const float d = screenPos[(size_t) k].getDistanceSquaredFrom (p);
                if (d < bestD)
                {
                    bestD = d;
                    best = visibleIdx[(size_t) k];
                }
            }
    return best;
}

//==============================================================================
void MapView::paint (juce::Graphics& g)
{
    const auto& t = SkLookAndFeel::themeOf (*this);
    if (dirty || cacheTheme != (int) t.id)
        rebuildCache();

    const auto bounds = getLocalBounds().toFloat();
    drawFrame (g, bounds, t);

    g.saveState();
    g.reduceClipRegion (mapArea().expanded (10.0f).getIntersection (bounds).toNearestInt());

    const auto& entries = *snapshot;

    // category names, faint, behind the dots
    if (zoom < 3.0f)
        for (int c = 0; c < kNumCategories; ++c)
        {
            if (! visible[(size_t) c])
                continue;
            const auto cat = (Category) c;
            const auto ctr = categoryCentre (cat);
            const auto p = toScreen (ctr.x, ctr.y);
            const auto col = t.category (cat);
            g.setFont (t.font (Theme::Role::Label, 15.0f));
            g.setColour (t.id == ThemeId::Y2KChrome ? col.darker (0.4f).withAlpha (0.55f)
                                                    : juce::Colours::white.withAlpha (t.id == ThemeId::Liquid ? 0.5f : 0.35f));
            g.drawText (juce::String (categoryName (cat)).toUpperCase(), juce::Rectangle<float> (160.0f, 22.0f).withCentre (p),
                        juce::Justification::centred);
        }

    g.drawImage (cache, cacheBounds.toFloat());

    const float r = dotRadius();
    const auto ringCol = t.id == ThemeId::Y2KChrome ? t.text : juce::Colours::white;
    auto valid = [&] (int i) { return juce::isPositiveAndBelow (i, (int) entries.size()); };

    // the similar sounds shown in the similarity bar
    for (int i : neighbours)
        if (valid (i))
        {
            const auto& e = entries[(size_t) i];
            drawDot (g, toScreen (e.x, e.y), juce::jmax (3.2f, r * 1.6f), t.category (e.category), t, true);
        }

    // the selected track's sample
    if (highlightedPath.isNotEmpty())
        for (int k = 0; k < (int) visibleIdx.size(); ++k)
            if (entries[(size_t) visibleIdx[(size_t) k]].path == highlightedPath)
            {
                const auto p = screenPos[(size_t) k];
                g.setColour (ringCol.withAlpha (0.55f));
                g.drawEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (p), 1.2f);
                break;
            }

    // the seed: big ring, like XO
    if (valid (seed))
    {
        const auto& e = entries[(size_t) seed];
        const auto p = toScreen (e.x, e.y);
        drawDot (g, p, juce::jmax (4.0f, r * 2.0f), t.category (e.category), t, true);
        g.setColour (ringCol);
        g.drawEllipse (juce::Rectangle<float> (40.0f, 40.0f).withCentre (p), 1.8f);
        if (t.sparkles)
            skinpaint::sparkle (g, p.translated (22.0f, -22.0f), 6.0f, t.id == ThemeId::Y2KChrome ? t.accent : juce::Colours::white);
    }

    // what is playing right now
    if (valid (playing) && playing != seed)
    {
        const auto& e = entries[(size_t) playing];
        g.setColour (ringCol.withAlpha (0.9f));
        g.drawEllipse (juce::Rectangle<float> (20.0f, 20.0f).withCentre (toScreen (e.x, e.y)), 1.5f);
    }

    // hover label
    if (valid (hovered) && gesture == Gesture::none)
    {
        const auto& e = entries[(size_t) hovered];
        const auto p = toScreen (e.x, e.y);
        g.setColour (ringCol);
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f + 6.0f, r * 2.0f + 6.0f).withCentre (p), 1.3f);

        const auto font = t.font (Theme::Role::UIBold, 14.0f);
        g.setFont (font);
        const auto text = e.name + "   " + juce::String (e.duration, 2) + " s";
        const float w = juce::jmin (420.0f, (float) juce::GlyphArrangement::getStringWidthInt (font, text) + 18.0f);
        auto box = juce::Rectangle<float> (w, 24.0f).withPosition (p.x + 12.0f, p.y - 34.0f).constrainedWithin (bounds.reduced (4.0f));
        switch (t.id)
        {
            case ThemeId::Liquid:
                g.setColour (juce::Colour (0xee1d1e6e)); g.fillRoundedRectangle (box, 12.0f);
                g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawRoundedRectangle (box, 12.0f, 1.0f);
                break;
            case ThemeId::Y2KChrome:
                g.setColour (juce::Colours::white.withAlpha (0.95f)); g.fillRoundedRectangle (box, 12.0f);
                g.setColour (juce::Colour (0xff9aa7c0)); g.drawRoundedRectangle (box, 12.0f, 1.0f);
                break;
            case ThemeId::Digicore:
                g.setColour (juce::Colour (0xff120a1f)); g.fillRect (box);
                g.setColour (t.accent2); g.drawRect (box, 1.0f);
                break;
            case ThemeId::PS2Night:
            case ThemeId::Mix:
                g.setColour (juce::Colour (0xee050914)); g.fillRoundedRectangle (box, 5.0f);
                g.setColour (t.accent.withAlpha (0.6f)); g.drawRoundedRectangle (box, 5.0f, 1.0f);
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
    else if (visibleCount == 0)
    {
        g.setColour (t.textDim);
        g.setFont (t.font (Theme::Role::UI, 16.0f));
        g.drawText ("No sounds match the current filter", mapArea(), juce::Justification::centred);
    }

    g.restoreState();

    if (statusText.isNotEmpty())
    {
        g.setColour (t.textDim);
        g.setFont (t.font (Theme::Role::Label, 11.0f));
        g.drawText (statusText, bounds.reduced (16.0f, 9.0f), juce::Justification::bottomLeft);
    }
}

//==============================================================================
void MapView::mouseMove (const juce::MouseEvent& e)
{
    const int h = findDot (e.position, 9.0f);
    if (h != hovered)
    {
        hovered = h;
        repaint();
    }
}

void MapView::mouseExit (const juce::MouseEvent&)
{
    if (hovered >= 0 && gesture == Gesture::none)
    {
        hovered = -1;
        repaint();
    }
}

void MapView::mouseDown (const juce::MouseEvent& e)
{
    panStartCentre = centre;
    lastScrub = -1;

    if (e.mods.isPopupMenu() || e.mods.isCommandDown() || e.mods.isMiddleButtonDown())
    {
        gesture = Gesture::pan;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        return;
    }

    gesture = Gesture::scrub;
    hovered = -1;
    const int hit = findDot (e.position, 12.0f);
    if (hit >= 0)
    {
        lastScrub = hit;
        lastScrubTime = juce::Time::getMillisecondCounter();
        if (onPick)
            onPick (hit, false);
    }
    repaint();
}

void MapView::mouseDrag (const juce::MouseEvent& e)
{
    if (gesture == Gesture::pan)
    {
        const float s = pixelsPerUnit();
        const auto d = e.position - e.mouseDownPosition;
        centre = { panStartCentre.x - d.x / s, panStartCentre.y - d.y / s };
        clampView();
        dirty = true;
        repaint();
        return;
    }

    if (gesture != Gesture::scrub)
        return;

    // leaving the map while scrubbing carries the last sound onto a track
    if (! getLocalBounds().contains (e.getPosition()) && lastScrub >= 0)
    {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
        {
            const auto& t = SkLookAndFeel::themeOf (*this);
            const auto& entry = (*snapshot)[(size_t) lastScrub];
            juce::Image img (juce::Image::ARGB, 22, 22, true);
            {
                juce::Graphics ig (img);
                drawDot (ig, { 11.0f, 11.0f }, 7.0f, t.category (entry.category), t, false);
            }
            gesture = Gesture::dragOut;
            container->startDragging (juce::var (entry.path), this, juce::ScaledImage (img));
        }
        return;
    }

    // scrub: play each new sound the cursor passes over (at most every 35 ms)
    const int hit = findDot (e.position, 10.0f);
    const auto now = juce::Time::getMillisecondCounter();
    if (hit >= 0 && hit != lastScrub && now - lastScrubTime >= 35)
    {
        lastScrub = hit;
        lastScrubTime = now;
        if (onPick)
            onPick (hit, true);
    }
}

void MapView::mouseUp (const juce::MouseEvent&)
{
    gesture = Gesture::none;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void MapView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = findDot (e.position, 12.0f);
    if (h >= 0)
    {
        if (onAssign)
            onAssign ((*snapshot)[(size_t) h].path);
    }
    else
    {
        zoom = 1.0f;
        clampView();
        dirty = true;
        repaint();
    }
}

void MapView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    // trackpad two-finger scroll moves around; a mouse wheel (or Cmd + scroll) zooms
    if (w.isSmooth && ! e.mods.isCommandDown())
    {
        const float s = pixelsPerUnit();
        centre -= juce::Point<float> (w.deltaX, w.deltaY) * (w.isReversed ? -1.0f : 1.0f) * (180.0f / s);
        clampView();
        dirty = true;
        repaint();
        return;
    }
    zoomAround (e.position, std::exp (w.deltaY * 1.4f));
}

void MapView::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    zoomAround (e.position, scaleFactor);
}
} // namespace sk
