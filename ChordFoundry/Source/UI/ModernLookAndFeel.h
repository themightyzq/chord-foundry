#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ChordFoundry {

/**
 * ModernLookAndFeel - Custom JUCE LookAndFeel implementing contemporary design principles
 * 
 * Features:
 * - Modern color palette with proper contrast ratios for accessibility
 * - Consistent typography hierarchy using system fonts
 * - Rounded corners and subtle shadows for depth
 * - High contrast mode support
 * - Keyboard focus indicators
 * - Professional spacing and layout
 */
class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel();
    ~ModernLookAndFeel() override = default;

    //==============================================================================
    // Color scheme
    struct Colors
    {
        // Primary colors
        static const juce::Colour primary;          // #2563EB (Blue 600)
        static const juce::Colour primaryDark;     // #1D4ED8 (Blue 700)  
        static const juce::Colour primaryLight;    // #3B82F6 (Blue 500)
        
        // Secondary colors
        static const juce::Colour secondary;       // #059669 (Emerald 600)
        static const juce::Colour secondaryDark;   // #047857 (Emerald 700)
        static const juce::Colour accent;          // #DC2626 (Red 600)
        
        // Neutral colors
        static const juce::Colour background;      // #FAFAF9 (Stone 50)
        static const juce::Colour surface;         // #FFFFFF (White)
        static const juce::Colour surfaceElevated;  // #F8FAFC (Slate 50)
        
        // Text colors
        static const juce::Colour textPrimary;     // #1F2937 (Gray 800)
        static const juce::Colour textSecondary;   // #6B7280 (Gray 500)
        static const juce::Colour textOnPrimary;   // #FFFFFF (White)
        
        // Border and outline colors
        static const juce::Colour border;          // #E5E7EB (Gray 200)
        static const juce::Colour borderFocus;     // #3B82F6 (Blue 500)
        static const juce::Colour borderError;     // #EF4444 (Red 500)
        
        // State colors
        static const juce::Colour hover;           // #F3F4F6 (Gray 100)
        static const juce::Colour pressed;         // #E5E7EB (Gray 200)
        static const juce::Colour disabled;        // #9CA3AF (Gray 400)
        
        // Semantic colors
        static const juce::Colour success;         // #10B981 (Emerald 500)
        static const juce::Colour warning;         // #F59E0B (Amber 500)
        static const juce::Colour error;           // #EF4444 (Red 500)
        static const juce::Colour info;            // #3B82F6 (Blue 500)
    };

    //==============================================================================
    // Typography - Modern font system with sleek typefaces
    struct Typography
    {
        static const juce::Font getHeaderFont();       // 32pt light
        static const juce::Font getSubheaderFont();    // 22pt regular
        static const juce::Font getBodyFont();         // 15pt regular
        static const juce::Font getCaptionFont();      // 13pt regular
        static const juce::Font getSmallFont();        // 11pt regular
        static const juce::Font getButtonFont();       // 15pt medium
        
        // Additional specialized fonts for modern UI
        static const juce::Font getMonospacedFont();   // For numeric displays
        static const juce::Font getDisplayFont();      // For large displays
        static const juce::Font getLabelFont();        // For UI labels
    };

    //==============================================================================
    // Spacing and sizing - Enhanced for modern audio application
    struct Metrics
    {
        static constexpr int cornerRadius = 8;        // Slightly more subtle corners
        static constexpr int borderWidth = 1;
        static constexpr int focusOutlineWidth = 2;
        static constexpr int shadowOffset = 4;        // Stronger shadow for depth
        static constexpr float shadowOpacity = 0.25f; // More pronounced shadows
        static constexpr float glowRadius = 6.0f;     // Glow effect for accents
        static constexpr float gradientIntensity = 0.15f; // Subtle gradient strength
        
        // Spacing (8px grid system)
        static constexpr int spacingXS = 4;   // 0.25rem
        static constexpr int spacingSM = 8;   // 0.5rem
        static constexpr int spacingMD = 16;  // 1rem
        static constexpr int spacingLG = 24;  // 1.5rem
        static constexpr int spacingXL = 32;  // 2rem
        static constexpr int spacing2XL = 48; // 3rem
        
        // Component heights
        static constexpr int buttonHeight = 44;
        static constexpr int inputHeight = 40;
        static constexpr int headerHeight = 60;
    };

    //==============================================================================
    // LookAndFeel overrides
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, 
                       bool shouldDrawButtonAsHighlighted, 
                       bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, 
                     bool isButtonDown, int buttonX, int buttonY, 
                     int buttonW, int buttonH, juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, 
                      int x, int y, int width, int height,
                      bool isScrollbarVertical, int thumbStartPosition, 
                      int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, 
                         juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, 
                         bool shouldDrawButtonAsDown) override;

    // Note: drawTabButton method signature varies between JUCE versions
    // Implementing the base class version for compatibility
    
    //==============================================================================
    // Accessibility and keyboard focus  
    void drawFocusRectangle(juce::Graphics& g, const juce::Rectangle<int>& area,
                           juce::Component& component);

    //==============================================================================
    // Utility methods
    static void drawRoundedRectWithShadow(juce::Graphics& g, 
                                         const juce::Rectangle<float>& bounds,
                                         const juce::Colour& fillColour,
                                         const juce::Colour& borderColour = Colors::border,
                                         float cornerRadius = Metrics::cornerRadius,
                                         bool drawShadow = true);

    static void drawCard(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                        bool isElevated = false);

    static void drawGradientBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     const juce::Colour& topColour, const juce::Colour& bottomColour,
                                     float cornerRadius = Metrics::cornerRadius);
    
    static void drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                              const juce::Colour& glowColour, float radius = Metrics::glowRadius,
                              float cornerRadius = Metrics::cornerRadius);
    
    static void drawModernButton(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                               const juce::Colour& baseColour, bool isPressed = false,
                               bool hasGlow = false, float cornerRadius = Metrics::cornerRadius);

    static juce::Colour getChordColour(const juce::String& romanNumeral);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernLookAndFeel)
};

} // namespace ChordFoundry