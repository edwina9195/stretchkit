#pragma once

#include "Common.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace sk
{
enum class ThemeId : int { PS2Night = 0, Y2KChrome, Digicore, Mix, Liquid };
constexpr int kNumThemes = 5;

enum class ButtonStyle { Glass, Chrome, Pixel };
enum class DotStyle    { Glow, Bubble, Square, ChromeBead };
enum class CellStyle   { GlassCube, Gel, Pixel };
enum class KnobStyle   { GlowRing, Chrome, Pixel };

/** Everything that changes between skins. */
struct Theme
{
    ThemeId id;
    const char* name;

    juce::Colour bgTop, bgBottom;      // window gradient
    juce::Colour panel, panelEdge;     // map / menus
    juce::Colour text, textDim;
    juce::Colour accent, accent2;      // main + secondary highlight
    juce::Colour cellOff, cellOffAlt;  // empty step cells (alternating groups of 4)
    juce::Colour buttonText;           // text on un-pressed buttons
    std::array<juce::Colour, kNumCategories> cat;

    ButtonStyle buttons;
    DotStyle dots;
    CellStyle cells;
    KnobStyle knobs;

    bool fog, towers, menuLights, scanlines, sparkles, glitch, chromeTitle, pixelGrid, iridescent, orbitRings;
    float uiScale;                     // pixel fonts need to be drawn larger
    bool chromeIcons = false;          // 3D liquid-chrome icons on buttons
    bool lightStreak = false;          // diagonal glass light streak across the window

    enum class Role { Title, UI, UIBold, Label };
    juce::Font font (Role role, float height) const;

    juce::Colour category (Category c) const { return cat[(size_t) c]; }
    juce::Colour onButtonText (juce::Colour base) const;

    static const Theme& get (ThemeId id);
    static const Theme& get (int index) { return get ((ThemeId) juce::jlimit (0, kNumThemes - 1, index)); }
};

/** Shared drawing helpers for the skins. */
namespace skinpaint
{
    void sparkle (juce::Graphics&, juce::Point<float> centre, float size, juce::Colour colour);
    void chromeFill (juce::Graphics&, const juce::Path&, juce::Rectangle<float> bounds, bool strong = false);
    void textPath (juce::Path&, const juce::String&, const juce::Font&, juce::Rectangle<float>, juce::Justification);
    void glitchText (juce::Graphics&, const juce::String&, const juce::Font&, juce::Rectangle<float>,
                     juce::Justification, juce::Colour main, float offset);
    void towers (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, int seed);
    void menuLights (juce::Graphics&, juce::Point<float> start, float spacing);
    void lightStreak (juce::Graphics&, juce::Rectangle<float> area, float strength);

    /** Pre-rendered liquid-chrome icon ("kick", "play", "dice", ...). dim = desaturated, faded. */
    juce::Image chromeIcon (const juce::String& name, bool dim = false);
}

//==============================================================================
class SkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SkLookAndFeel();

    void setTheme (ThemeId id);
    const Theme& theme() const noexcept { return *current; }

    /** Theme of whatever look-and-feel a component is using (falls back to the default skin). */
    static const Theme& themeOf (const juce::Component& c);

    /** The skin's button shape, also used for combo boxes and custom components. */
    void drawSkinButton (juce::Graphics&, juce::Rectangle<float> r, juce::Colour base,
                         bool on, bool over, bool down) const;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label&) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;

    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;

private:
    const Theme* current;
};
} // namespace sk
