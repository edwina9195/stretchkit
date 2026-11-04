#include "Theme.h"

#include "BinaryData.h"

#include <map>

namespace sk
{
//==============================================================================
namespace
{
struct EmbeddedFonts
{
    juce::Typeface::Ptr rajMedium, rajSemi, rajBold, michroma, vt323, silkscreen, pressStart, inter;

    static const EmbeddedFonts& get()
    {
        static EmbeddedFonts f;
        return f;
    }

private:
    EmbeddedFonts()
    {
        auto load = [] (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); };
        rajMedium  = load (BinaryData::RajdhaniMedium_ttf,       BinaryData::RajdhaniMedium_ttfSize);
        rajSemi    = load (BinaryData::RajdhaniSemiBold_ttf,     BinaryData::RajdhaniSemiBold_ttfSize);
        rajBold    = load (BinaryData::RajdhaniBold_ttf,         BinaryData::RajdhaniBold_ttfSize);
        michroma   = load (BinaryData::MichromaRegular_ttf,      BinaryData::MichromaRegular_ttfSize);
        vt323      = load (BinaryData::VT323Regular_ttf,         BinaryData::VT323Regular_ttfSize);
        silkscreen = load (BinaryData::SilkscreenRegular_ttf,    BinaryData::SilkscreenRegular_ttfSize);
        pressStart = load (BinaryData::PressStart2PRegular_ttf,  BinaryData::PressStart2PRegular_ttfSize);
        inter      = load (BinaryData::InterVariable_ttf,        BinaryData::InterVariable_ttfSize);
    }
};

juce::Font makeFont (juce::Typeface::Ptr tf, float height, float kerning = 0.0f)
{
    return juce::Font (juce::FontOptions().withTypeface (tf).withHeight (height).withKerningFactor (kerning));
}

using C = juce::Colour;

const Theme themes[kNumThemes] = {
    // ---------------------------------------------------------------- PS2 NIGHT
    { ThemeId::PS2Night, "PS2 Night",
      C (0xff03050c), C (0xff0b1a40), C (0xff071028), C (0xff2f56b8),
      C (0xffd6e4ff), C (0xff7088bb), C (0xff4d8dff), C (0xffa8d4ff),
      C (0xff0e1836), C (0xff0b1430), C (0xffd6e4ff),
      { C (0xffff4f6e), C (0xffffa94d), C (0xfff2e15c), C (0xff3fe0c5),
        C (0xff5fa8ff), C (0xffa57bff), C (0xffff76c8), C (0xff8b98bd) },
      ButtonStyle::Glass, DotStyle::Glow, CellStyle::GlassCube, KnobStyle::GlowRing,
      /*fog*/ true, /*towers*/ true, /*menuLights*/ true, /*scanlines*/ true, /*sparkles*/ false,
      /*glitch*/ false, /*chromeTitle*/ false, /*pixelGrid*/ false, /*iridescent*/ false, /*orbit*/ false,
      1.1f },

    // ---------------------------------------------------------------- Y2K CHROME
    { ThemeId::Y2KChrome, "Y2K Chrome",
      C (0xfff4f7fd), C (0xffc8d4e8), C (0xffffffff), C (0xff9aa7c0),
      C (0xff18203d), C (0xff5c6a8e), C (0xffff3fa4), C (0xff21b0ff),
      C (0xffe9eef7), C (0xffdde5f2), C (0xff18203d),
      { C (0xffff3d7f), C (0xffff8a1c), C (0xffd9bd00), C (0xff00bfa5),
        C (0xff1f8fff), C (0xff8a57ff), C (0xffff45d2), C (0xff8d98ae) },
      ButtonStyle::Chrome, DotStyle::Bubble, CellStyle::Gel, KnobStyle::Chrome,
      false, false, false, false, true,
      false, true, false, true, true,
      1.1f, /*chromeIcons*/ true },

    // ---------------------------------------------------------------- DIGICORE
    { ThemeId::Digicore, "Digicore",
      C (0xff050008), C (0xff0e0019), C (0xff0a0512), C (0xff00f0ff),
      C (0xffeafffb), C (0xff9a86c4), C (0xffff2bd6), C (0xff00f0ff),
      C (0xff150b24), C (0xff1b0f2e), C (0xffeafffb),
      { C (0xffff2b6a), C (0xffff8a00), C (0xfffff200), C (0xff00ffd0),
        C (0xff00a8ff), C (0xffb05cff), C (0xffff2bd6), C (0xffb6ff00) },
      ButtonStyle::Pixel, DotStyle::Square, CellStyle::Pixel, KnobStyle::Pixel,
      false, false, false, true, true,
      true, false, true, false, false,
      1.35f },

    // ---------------------------------------------------------------- MIX
    { ThemeId::Mix, "Mix",
      C (0xff04060f), C (0xff101a42), C (0xff070f26), C (0xff7fa6e6),
      C (0xffe2ecff), C (0xff7d93c4), C (0xff38d3ff), C (0xffff4fd8),
      C (0xff0e1836), C (0xff0b1430), C (0xff18203d),
      { C (0xffff4f7b), C (0xffffa94d), C (0xfff2e15c), C (0xff3fe8cf),
        C (0xff5fb0ff), C (0xffae84ff), C (0xffff6fd6), C (0xff98a6c8) },
      ButtonStyle::Chrome, DotStyle::Glow, CellStyle::GlassCube, KnobStyle::Chrome,
      true, true, false, true, true,
      true, true, false, false, false,
      1.1f, /*chromeIcons*/ true },

    // ---------------------------------------------------------------- LIQUID CHROME
    { ThemeId::Liquid, "Liquid Chrome",
      C (0xff3b3ec4), C (0xff25268c), C (0xff22238a), C (0xffd2d0ff),
      C (0xffffffff), C (0xffc6c8ff), C (0xffa9f0ff), C (0xffff9ade),
      C (0xff3936a6), C (0xff33319a), C (0xff18203d),
      { C (0xffff8fb1), C (0xffffc27a), C (0xfffff07a), C (0xff8af4e4),
        C (0xff9cd2ff), C (0xffc9a8ff), C (0xffff9ade), C (0xffe6e8ff) },
      ButtonStyle::Chrome, DotStyle::ChromeBead, CellStyle::Gel, KnobStyle::Chrome,
      false, false, false, false, true,
      false, true, false, false, false,
      1.0f, /*chromeIcons*/ true, /*lightStreak*/ true },
};
} // namespace

const Theme& Theme::get (ThemeId id)
{
    return themes[juce::jlimit (0, kNumThemes - 1, (int) id)];
}

juce::Font Theme::font (Role role, float h) const
{
    const auto& f = EmbeddedFonts::get();
    const float ui = h * uiScale;

    switch (id)
    {
        case ThemeId::PS2Night:
            switch (role)
            {
                case Role::Title:  return makeFont (f.rajBold, h, 0.32f);
                case Role::UI:     return makeFont (f.rajMedium, ui);
                case Role::UIBold: return makeFont (f.rajSemi, ui);
                case Role::Label:  return makeFont (f.rajSemi, ui, 0.14f);
            }
            break;

        case ThemeId::Y2KChrome:
            switch (role)
            {
                case Role::Title:  return makeFont (f.michroma, h, 0.06f);
                case Role::UI:     return makeFont (f.rajSemi, ui);
                case Role::UIBold: return makeFont (f.rajBold, ui);
                case Role::Label:  return makeFont (f.michroma, h * 0.82f, 0.04f);
            }
            break;

        case ThemeId::Digicore:
            switch (role)
            {
                case Role::Title:  return makeFont (f.pressStart, h * 0.8f);
                case Role::UI:     return makeFont (f.vt323, ui);
                case Role::UIBold: return makeFont (f.vt323, ui);
                case Role::Label:  return makeFont (f.silkscreen, h * 0.86f);
            }
            break;

        case ThemeId::Liquid:
            switch (role)
            {
                case Role::Title:  return makeFont (f.michroma, h, 0.08f);
                case Role::UI:     return makeFont (f.inter, ui * 0.92f);
                case Role::UIBold: return makeFont (f.inter, ui * 0.92f);
                case Role::Label:  return makeFont (f.inter, ui * 0.86f, 0.06f);
            }
            break;

        case ThemeId::Mix:
            switch (role)
            {
                case Role::Title:  return makeFont (f.michroma, h, 0.08f);
                case Role::UI:     return makeFont (f.rajMedium, ui);
                case Role::UIBold: return makeFont (f.rajSemi, ui);
                case Role::Label:  return makeFont (f.silkscreen, h * 0.86f);
            }
            break;
    }
    return makeFont (f.rajMedium, ui);
}

juce::Colour Theme::onButtonText (juce::Colour) const
{
    return buttons == ButtonStyle::Pixel ? juce::Colour (0xff0b0014) : juce::Colours::white;
}

//==============================================================================
namespace skinpaint
{
void sparkle (juce::Graphics& g, juce::Point<float> c, float s, juce::Colour colour)
{
    juce::Path p;
    const float k = s * 0.16f;
    p.startNewSubPath (c.x, c.y - s);
    p.quadraticTo (c.x + k, c.y - k, c.x + s, c.y);
    p.quadraticTo (c.x + k, c.y + k, c.x, c.y + s);
    p.quadraticTo (c.x - k, c.y + k, c.x - s, c.y);
    p.quadraticTo (c.x - k, c.y - k, c.x, c.y - s);
    p.closeSubPath();

    g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.35f), c, colour.withAlpha (0.0f), c.translated (s * 0.9f, 0.0f), true));
    g.fillEllipse (juce::Rectangle<float> (s * 1.8f, s * 1.8f).withCentre (c));
    g.setColour (colour);
    g.fillPath (p);
}

void chromeFill (juce::Graphics& g, const juce::Path& p, juce::Rectangle<float> b, bool strong)
{
    // liquid chrome: white sky, lavender, a pink/teal iridescent edge, a dark horizon band, light ground
    juce::ColourGradient grad (juce::Colour (0xffffffff), 0.0f, b.getY(), juce::Colour (0xffe9e8ff), 0.0f, b.getBottom(), false);
    grad.addColour (0.22, juce::Colour (0xfff1efff));
    grad.addColour (0.38, juce::Colour (0xffc9c4ff));
    grad.addColour (0.45, juce::Colour (0xffffc6ec));
    // full-strength horizon band for big chrome (titles); softer on buttons so their text stays readable
    grad.addColour (0.50, strong ? juce::Colour (0xff15163f) : juce::Colour (0xff9a9dd8));
    grad.addColour (0.56, strong ? juce::Colour (0xff4c4fb8) : juce::Colour (0xffb8bbec));
    grad.addColour (0.70, juce::Colour (0xffa9f0ff));
    grad.addColour (0.86, juce::Colour (0xfff6f5ff));
    g.setGradientFill (grad);
    g.fillPath (p);
}

void textPath (juce::Path& out, const juce::String& text, const juce::Font& font, juce::Rectangle<float> r, juce::Justification j)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, 0.0f, 0.0f);
    ga.justifyGlyphs (0, ga.getNumGlyphs(), r.getX(), r.getY(), r.getWidth(), r.getHeight(), j);
    ga.createPath (out);
}

void glitchText (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Rectangle<float> r,
                 juce::Justification j, juce::Colour main, float offset)
{
    g.setFont (font);
    g.setColour (juce::Colour (0xff00f0ff).withAlpha (0.75f));
    g.drawText (text, r.translated (-offset, 0.0f), j, false);
    g.setColour (juce::Colour (0xffff2bd6).withAlpha (0.75f));
    g.drawText (text, r.translated (offset, 0.0f), j, false);
    g.setColour (main);
    g.drawText (text, r, j, false);
}

void towers (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, int seed)
{
    juce::Random rng (seed);
    const int count = 11;
    for (int i = 0; i < count; ++i)
    {
        const float w = area.getWidth() * (0.025f + 0.03f * rng.nextFloat());
        const float h = area.getHeight() * (0.25f + 0.6f * rng.nextFloat());
        const float x = area.getX() + area.getWidth() * ((float) i + 0.15f + 0.7f * rng.nextFloat()) / (float) count - w * 0.5f;
        const juce::Rectangle<float> col (x, area.getBottom() - h, w, h);

        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.02f), 0.0f, col.getY(),
                                                 colour.withAlpha (0.24f), 0.0f, col.getBottom(), false));
        g.fillRect (col);
        g.setColour (colour.withAlpha (0.30f));
        g.fillRect (col.withWidth (1.0f));
        g.setColour (colour.withAlpha (0.18f));
        g.fillRect (col.withHeight (2.0f));
    }
}

void menuLights (juce::Graphics& g, juce::Point<float> start, float spacing)
{
    const juce::uint32 colours[] = { 0xffff4040, 0xff40a0ff, 0xff40ff8a, 0xffffd040 };
    for (int i = 0; i < 4; ++i)
    {
        const auto c = start.translated (spacing * (float) i, 0.0f);
        const auto col = juce::Colour (colours[i]);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f), c, col.withAlpha (0.0f), c.translated (9.0f, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (c));
        g.setColour (col.brighter (0.4f));
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (c));
    }
}
void lightStreak (juce::Graphics& g, juce::Rectangle<float> area, float strength)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (area.toNearestInt());
    const auto c = area.getCentre();
    const float len = area.getWidth() + area.getHeight();
    const auto rot = juce::AffineTransform::rotation (-0.9f, c.x, c.y);

    auto band = [&] (float offset, float width, float alpha)
    {
        const auto r = juce::Rectangle<float> (width, len).withCentre (c.translated (offset, 0.0f));
        juce::ColourGradient grad (juce::Colours::white.withAlpha (0.0f), r.getX(), 0.0f,
                                   juce::Colours::white.withAlpha (0.0f), r.getRight(), 0.0f, false);
        grad.addColour (0.5, juce::Colours::white.withAlpha (alpha * strength));
        grad.point1 = grad.point1.transformedBy (rot);
        grad.point2 = grad.point2.transformedBy (rot);
        g.setGradientFill (grad);
        juce::Path p;
        p.addRectangle (r);
        g.fillPath (p, rot);
    };

    band (-area.getWidth() * 0.12f, area.getWidth() * 0.16f, 0.22f);
    band (-area.getWidth() * 0.06f, area.getWidth() * 0.018f, 0.32f);
}

juce::Image chromeIcon (const juce::String& name, bool dim)
{
    struct Cache
    {
        std::map<juce::String, juce::Image> normal, dimmed;
    };
    static Cache cache;

    auto& map = dim ? cache.dimmed : cache.normal;
    if (auto it = map.find (name); it != map.end())
        return it->second;

    int size = 0;
    const auto resource = "icon_" + name + "_png";
    const char* data = BinaryData::getNamedResource (resource.toRawUTF8(), size);
    auto img = data != nullptr ? juce::ImageCache::getFromMemory (data, size) : juce::Image();

    if (dim && img.isValid())
    {
        img = img.createCopy();
        img.desaturate();
        img.multiplyAllAlphas (0.35f);
    }
    map[name] = img;
    return img;
}
} // namespace skinpaint

//==============================================================================
SkLookAndFeel::SkLookAndFeel()
{
    setTheme (ThemeId::Mix);
}

const Theme& SkLookAndFeel::themeOf (const juce::Component& c)
{
    if (auto* l = dynamic_cast<SkLookAndFeel*> (&c.getLookAndFeel()))
        return l->theme();
    return Theme::get (ThemeId::Mix);
}

void SkLookAndFeel::setTheme (ThemeId id)
{
    current = &Theme::get (id);
    const auto& t = *current;
    const bool light = t.id == ThemeId::Y2KChrome;

    setColourScheme ({ t.bgTop, t.panel, t.panel, t.panelEdge, t.text, t.accent, juce::Colours::white, t.accent, t.text });

    setColour (juce::TextButton::buttonColourId, t.panel);
    setColour (juce::TextButton::buttonOnColourId, t.accent);
    setColour (juce::TextButton::textColourOffId, t.buttonText);
    setColour (juce::TextButton::textColourOnId, t.onButtonText (t.accent));
    setColour (juce::ComboBox::textColourId, t.buttonText);
    setColour (juce::ComboBox::arrowColourId, t.accent2);
    setColour (juce::Label::textColourId, t.text);
    setColour (juce::ToggleButton::textColourId, t.text);
    setColour (juce::Slider::textBoxTextColourId, light ? t.text : t.text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, t.accent);
    setColour (juce::PopupMenu::backgroundColourId, light ? juce::Colour (0xfff7f9fd) : t.panel);
    setColour (juce::PopupMenu::textColourId, t.text);
    setColour (juce::PopupMenu::headerTextColourId, t.textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent);
    setColour (juce::PopupMenu::highlightedTextColourId, t.onButtonText (t.accent));
    setColour (juce::TooltipWindow::backgroundColourId, light ? juce::Colour (0xfff7f9fd) : t.panel.darker (0.3f));
    setColour (juce::TooltipWindow::textColourId, t.text);
    setColour (juce::TooltipWindow::outlineColourId, t.panelEdge);
    setColour (juce::BubbleComponent::backgroundColourId, light ? juce::Colour (0xfff7f9fd) : t.panel);
    setColour (juce::BubbleComponent::outlineColourId, t.panelEdge);
}

//==============================================================================
void SkLookAndFeel::drawSkinButton (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base,
                                    bool on, bool over, bool down) const
{
    const auto& t = *current;

    switch (t.buttons)
    {
        case ButtonStyle::Glass:
        {
            const float rad = 5.0f;
            if (on)
            {
                g.setColour (base.withAlpha (0.22f));
                g.fillRoundedRectangle (r.expanded (2.5f), rad + 2.0f);
            }
            const auto top = on ? base.withAlpha (0.55f) : juce::Colour (0xff1a2b58).withAlpha (over ? 0.95f : 0.8f);
            const auto bottom = on ? base.darker (0.5f).withAlpha (0.65f) : juce::Colour (0xff0a1230).withAlpha (0.85f);
            g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (on ? base.brighter (0.3f) : t.accent.withAlpha (over ? 0.8f : 0.45f));
            g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (down ? 0.05f : 0.14f));
            g.fillRect (r.reduced (rad, 0.0f).withHeight (1.0f).translated (0.0f, 1.5f));
            break;
        }

        case ButtonStyle::Chrome:
        {
            const float rad = r.getWidth() < r.getHeight() * 1.3f ? r.getHeight() * 0.32f : r.getHeight() * 0.5f;
            juce::Path shape;
            shape.addRoundedRectangle (r, rad);

            if (on)
            {
                juce::ColourGradient gel (base.brighter (0.55f), 0.0f, r.getY(), base.darker (0.25f), 0.0f, r.getBottom(), false);
                gel.addColour (0.55, base);
                g.setGradientFill (gel);
                g.fillPath (shape);
                g.setColour (base.darker (0.7f));
            }
            else
            {
                skinpaint::chromeFill (g, shape, r);
                if (over)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.25f));
                    g.fillPath (shape);
                }
                g.setColour (t.id == ThemeId::Mix ? juce::Colour (0xff0a1026)
                           : t.id == ThemeId::Liquid ? juce::Colour (0xff14154a) : juce::Colour (0xff5d677d));
            }
            g.strokePath (shape, juce::PathStrokeType (1.0f));

            // gloss
            auto gloss = r.reduced (rad * 0.45f, 1.5f).withHeight (r.getHeight() * 0.42f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.2f : 0.6f), 0.0f, gloss.getY(),
                                                     juce::Colours::white.withAlpha (0.0f), 0.0f, gloss.getBottom(), false));
            g.fillRoundedRectangle (gloss, gloss.getHeight() * 0.5f);

            if (down)
            {
                g.setColour (juce::Colours::black.withAlpha (0.15f));
                g.fillPath (shape);
            }
            break;
        }

        case ButtonStyle::Pixel:
        {
            auto body = r.reduced (0.0f, 0.0f).withTrimmedRight (3.0f).withTrimmedBottom (3.0f);
            if (down)
                body = body.translated (2.0f, 2.0f);
            else
            {
                g.setColour (t.accent);
                g.fillRect (body.translated (3.0f, 3.0f));
            }
            g.setColour (on ? base : juce::Colour (0xff120a1f));
            g.fillRect (body);
            g.setColour (on ? juce::Colours::white : over ? juce::Colour (0xffb6ff00) : t.accent2);
            g.drawRect (body, 2.0f);
            break;
        }
    }
}

juce::Font SkLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return current->font (Theme::Role::UIBold, juce::jmin (15.0f, (float) buttonHeight * 0.56f));
}

namespace
{
juce::String iconOf (const juce::Button& b)    { return b.getProperties()["icon"].toString(); }
juce::String layoutOf (const juce::Button& b)  { return b.getProperties()["iconLayout"].toString(); }
} // namespace

void SkLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg, bool over, bool down)
{
    const auto& t = *current;
    if (t.chromeIcons && iconOf (b).isNotEmpty())
    {
        // icon buttons have no plate - just a soft halo, like a console menu
        const bool on = b.getToggleState() || layoutOf (b) != "stack";
        auto r = b.getLocalBounds().toFloat();
        if (layoutOf (b) == "stack")
            r = r.withHeight (r.getHeight() - 11.0f);
        const auto c = layoutOf (b) == "inline" ? juce::Point<float> (r.getX() + r.getHeight() * 0.5f, r.getCentreY()) : r.getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.62f;
        const auto glow = b.getToggleState() && layoutOf (b) == "stack" ? bg : (t.id == ThemeId::Y2KChrome ? t.accent2 : juce::Colours::white);
        const float a = (on && b.getToggleState() ? 0.32f : 0.0f) + (over ? 0.22f : 0.0f) + (down ? 0.1f : 0.0f);
        if (a > 0.0f)
        {
            g.setGradientFill (juce::ColourGradient (glow.withAlpha (a), c, glow.withAlpha (0.0f), c.translated (rad, 0.0f), true));
            g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c));
        }
        return;
    }

    drawSkinButton (g, b.getLocalBounds().toFloat().reduced (1.0f), bg, b.getToggleState(), over, down);
}

void SkLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const auto& t = *current;
    const auto icon = iconOf (b);

    if (t.chromeIcons && icon.isNotEmpty())
    {
        const auto layout = layoutOf (b);
        const bool dim = layout == "stack" && ! b.getToggleState();
        const auto img = skinpaint::chromeIcon (icon, dim);
        const float shrink = down ? 0.9f : 1.0f;
        auto r = b.getLocalBounds().toFloat();

        auto drawIcon = [&] (juce::Rectangle<float> box)
        {
            box = box.withSizeKeepingCentre (box.getWidth() * shrink, box.getHeight() * shrink);
            if (! dim)
            {
                g.setColour (juce::Colour (0xff10104a).withAlpha (t.id == ThemeId::Y2KChrome ? 0.18f : 0.35f));
                g.drawImage (img, box.translated (1.5f, 2.5f), juce::RectanglePlacement::centred, true);
            }
            g.setOpacity (1.0f);
            g.drawImage (img, box, juce::RectanglePlacement::centred);
        };

        if (layout == "stack")
        {
            auto labelArea = r.removeFromBottom (11.0f);
            const float s = juce::jmin (r.getWidth() * 0.8f, r.getHeight());
            drawIcon (juce::Rectangle<float> (s, s).withCentre (r.getCentre()));

            const auto onCol = t.id == ThemeId::Liquid ? juce::Colours::white
                             : t.id == ThemeId::Y2KChrome ? b.findColour (juce::TextButton::buttonOnColourId).darker (0.3f)
                                                          : b.findColour (juce::TextButton::buttonOnColourId).brighter (0.2f);
            g.setColour (b.getToggleState() ? onCol : t.textDim.withAlpha (0.55f));
            g.setFont (t.font (Theme::Role::Label, 9.5f));
            g.drawText (b.getButtonText().toUpperCase(), labelArea, juce::Justification::centred, false);
        }
        else if (layout == "only")
        {
            const float s = juce::jmin (r.getWidth(), r.getHeight()) + 2.0f;
            drawIcon (juce::Rectangle<float> (s, s).withCentre (r.getCentre()));
        }
        else
        {
            const float s = r.getHeight() + 2.0f;
            drawIcon (r.removeFromLeft (s).withSizeKeepingCentre (s, s));
            r.removeFromLeft (3.0f);
            g.setColour (t.text);
            g.setFont (t.font (Theme::Role::UIBold, 14.0f));
            g.drawText (b.getButtonText().toUpperCase(), r, juce::Justification::centredLeft, false);
        }
        return;
    }

    auto r = b.getLocalBounds().toFloat().reduced (2.0f, 0.0f);
    if (current->buttons == ButtonStyle::Pixel)
        r = r.withTrimmedRight (3.0f).withTrimmedBottom (3.0f).translated (down ? 2.0f : 0.0f, down ? 2.0f : 0.0f);

    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1, 0.8f);
}

void SkLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto& t = *current;
    const bool on = b.getToggleState();
    const float s = 14.0f;
    const auto box = juce::Rectangle<float> (2.0f, ((float) b.getHeight() - s) * 0.5f, s, s);

    switch (t.buttons)
    {
        case ButtonStyle::Glass:
            if (on)
            {
                g.setColour (t.accent.withAlpha (0.3f));
                g.fillRoundedRectangle (box.expanded (2.0f), 4.0f);
                g.setColour (t.accent);
                g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
            }
            g.setColour (t.accent.withAlpha (over ? 0.9f : 0.6f));
            g.drawRoundedRectangle (box, 3.0f, 1.0f);
            break;

        case ButtonStyle::Chrome:
        {
            juce::Path p;
            p.addEllipse (box);
            if (on)
            {
                g.setGradientFill (juce::ColourGradient (t.accent2.brighter (0.6f), box.getX(), box.getY(),
                                                         t.accent2.darker (0.2f), box.getRight(), box.getBottom(), false));
                g.fillPath (p);
            }
            else
                skinpaint::chromeFill (g, p, box);
            g.setColour (juce::Colour (0xff3b4357));
            g.strokePath (p, juce::PathStrokeType (1.0f));
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillEllipse (box.reduced (4.0f, 3.0f).withHeight (4.0f).translated (0.0f, -1.0f));
            break;
        }

        case ButtonStyle::Pixel:
            g.setColour (juce::Colour (0xff120a1f));
            g.fillRect (box);
            g.setColour (over ? juce::Colour (0xffb6ff00) : t.accent2);
            g.drawRect (box, 2.0f);
            if (on)
            {
                g.setColour (juce::Colour (0xffb6ff00));
                g.fillRect (box.reduced (4.0f));
            }
            break;
    }

    g.setColour (b.findColour (juce::ToggleButton::textColourId));
    g.setFont (current->font (Theme::Role::UIBold, 14.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds().toFloat().withTrimmedLeft (s + 8.0f),
                juce::Justification::centredLeft, false);
}

//==============================================================================
void SkLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& slider)
{
    const auto& t = *current;
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.5f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    auto arc = [&] (float r, float a0, float a1, float width, juce::Colour col)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, juce::jmin (a0, a1), juce::jmax (a0, a1), true);
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    switch (t.knobs)
    {
        case KnobStyle::GlowRing:
        {
            const float r = radius - 2.0f;
            arc (r, startAngle, endAngle, 2.0f, juce::Colour (0xff1c2b58));
            arc (r, from, angle, 6.0f, t.accent.withAlpha (0.22f));
            arc (r, from, angle, 2.0f, t.accent2);
            const auto tip = c.getPointOnCircumference (r, angle);
            g.setColour (juce::Colours::white);
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (tip));
            g.setColour (t.accent.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (radius * 0.7f, radius * 0.7f).withCentre (c));
            break;
        }

        case KnobStyle::Chrome:
        {
            const float ringR = radius - 1.5f;
            arc (ringR, startAngle, endAngle, 2.5f, t.id == ThemeId::Mix ? juce::Colour (0xff1c2b58)
                                                  : t.id == ThemeId::Liquid ? juce::Colours::white.withAlpha (0.22f)
                                                                            : juce::Colour (0xffc3cad8));
            arc (ringR, from, angle, 2.5f, t.accent);

            const auto body = juce::Rectangle<float> (radius * 1.38f, radius * 1.38f).withCentre (c);
            juce::ColourGradient grad (juce::Colours::white, body.getX(), body.getY(), juce::Colour (0xff8892a6),
                                       body.getRight(), body.getBottom(), false);
            grad.addColour (0.5, juce::Colour (0xffd5dbe6));
            g.setGradientFill (grad);
            g.fillEllipse (body);
            g.setColour (juce::Colour (0xff3b4357));
            g.drawEllipse (body, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillEllipse (body.reduced (body.getWidth() * 0.28f, body.getHeight() * 0.18f).withHeight (body.getHeight() * 0.18f));

            const auto tip = c.getPointOnCircumference (body.getWidth() * 0.42f, angle);
            g.setColour (juce::Colour (0xff18203d));
            g.drawLine ({ c.getPointOnCircumference (body.getWidth() * 0.12f, angle), tip }, 2.0f);
            break;
        }

        case KnobStyle::Pixel:
        {
            const int dots = 11;
            const float r = radius - 2.0f;
            for (int i = 0; i < dots; ++i)
            {
                const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (dots - 1);
                const bool lit = bipolar ? (a >= juce::jmin (from, angle) - 0.01f && a <= juce::jmax (from, angle) + 0.01f)
                                         : a <= angle + 0.01f;
                const auto p = c.getPointOnCircumference (r, a);
                g.setColour (lit ? t.accent : juce::Colour (0xff2c1d46));
                g.fillRect (juce::Rectangle<float> (3.0f, 3.0f).withCentre (p).toNearestInt().toFloat());
            }
            const auto tip = c.getPointOnCircumference (r * 0.45f, angle);
            g.setColour (t.accent2);
            g.fillRect (juce::Rectangle<float> (4.0f, 4.0f).withCentre (tip).toNearestInt().toFloat());
            g.fillRect (juce::Rectangle<float> (2.0f, 2.0f).withCentre (c).toNearestInt().toFloat());
            break;
        }
    }
}

void SkLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearBar)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, slider);
        return;
    }

    const auto& t = *current;
    const auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float zeroX = bipolar ? r.getX() + r.getWidth() * (float) slider.valueToProportionOfLength (0.0) : r.getX();
    const auto fill = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (zeroX, pos), r.getY(), juce::jmax (zeroX, pos), r.getBottom());

    switch (t.buttons)
    {
        case ButtonStyle::Glass:
        {
            g.setColour (juce::Colour (0xff0a1230).withAlpha (0.9f));
            g.fillRoundedRectangle (r, 4.0f);
            g.setGradientFill (juce::ColourGradient (t.accent.withAlpha (0.6f), 0.0f, r.getY(), t.accent.withAlpha (0.25f), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (fill, 4.0f);
            g.setColour (t.accent.withAlpha (0.45f));
            g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRect (r.reduced (4.0f, 0.0f).withHeight (1.0f).translated (0.0f, 1.5f));
            break;
        }

        case ButtonStyle::Chrome:
        {
            const bool dark = t.id == ThemeId::Mix || t.id == ThemeId::Liquid;
            const float rad = r.getHeight() * 0.5f;
            g.setColour (t.id == ThemeId::Liquid ? juce::Colour (0xff15165a).withAlpha (0.55f)
                         : dark ? juce::Colour (0xff0a1230).withAlpha (0.92f) : juce::Colours::white.withAlpha (0.75f));
            g.fillRoundedRectangle (r, rad);

            const auto gelCol = t.id == ThemeId::Liquid ? juce::Colour (0xff7a86ff) : dark ? t.accent : t.accent2;
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (r, rad);
            g.reduceClipRegion (clip);
            juce::ColourGradient gel (gelCol.brighter (0.5f).withAlpha (0.9f), 0.0f, r.getY(), gelCol.darker (0.2f).withAlpha (0.9f), 0.0f, r.getBottom(), false);
            g.setGradientFill (gel);
            g.fillRect (fill);
            {
                juce::Graphics::ScopedSaveState fillClip (g);
                g.reduceClipRegion (fill.toNearestInt());
                g.setColour (juce::Colours::white.withAlpha (dark ? 0.32f : 0.5f));
                g.fillRoundedRectangle (r.reduced (rad * 0.5f, 1.5f).withHeight (r.getHeight() * 0.38f), r.getHeight() * 0.19f);
            }
            g.setColour (t.id == ThemeId::Liquid ? juce::Colours::white.withAlpha (0.45f)
                         : dark ? juce::Colour (0xff7f93bd).withAlpha (0.8f) : juce::Colour (0xff9aa7c0));
            g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
            break;
        }

        case ButtonStyle::Pixel:
        {
            g.setColour (juce::Colour (0xff0d0716));
            g.fillRect (r);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (fill.toNearestInt());
                g.setColour (t.accent.withAlpha (0.85f));
                for (float sx = r.getX() - r.getHeight(); sx < r.getRight(); sx += 5.0f)
                    g.drawLine (sx, r.getBottom(), sx + r.getHeight(), r.getY(), 2.0f);
            }
            g.setColour (t.accent2);
            g.drawRect (r, 1.0f);
            break;
        }
    }
}

juce::Font SkLookAndFeel::getLabelFont (juce::Label& l)
{
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
        return current->font (Theme::Role::UIBold, 15.0f);
    return l.getFont();
}

void SkLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox& box)
{
    const auto& t = *current;
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    drawSkinButton (g, r, t.accent, false, box.isMouseOver (true), down);

    const auto arrowArea = juce::Rectangle<float> ((float) w - (float) h, 0.0f, (float) h, (float) h).reduced ((float) h * 0.34f);

    if (t.buttons == ButtonStyle::Pixel)
    {
        g.setColour (t.accent2);
        const float px = 2.0f;
        const auto cx = std::round (arrowArea.getCentreX() - 4.0f);
        const auto cy = std::round (arrowArea.getCentreY() - 2.0f);
        for (int row = 0; row < 3; ++row)
            g.fillRect (cx + (float) row * px, cy + (float) row * px, (float) (4 - row) * px * 2.0f - (float) row * px, px);
        return;
    }

    juce::Path chevron;
    chevron.startNewSubPath (arrowArea.getX(), arrowArea.getY() + arrowArea.getHeight() * 0.3f);
    chevron.lineTo (arrowArea.getCentreX(), arrowArea.getBottom() - arrowArea.getHeight() * 0.2f);
    chevron.lineTo (arrowArea.getRight(), arrowArea.getY() + arrowArea.getHeight() * 0.3f);
    g.setColour (t.buttons == ButtonStyle::Chrome ? juce::Colour (0xff2a3350) : t.accent2);
    g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font SkLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return current->font (Theme::Role::UIBold, 15.0f);
}

juce::Font SkLookAndFeel::getPopupMenuFont()
{
    return current->font (Theme::Role::UI, 16.0f);
}

void SkLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (current->panelEdge.withAlpha (0.8f));
    g.drawRect (0, 0, w, h, 1);
}
} // namespace sk
