#include "ModernLookAndFeel.h"

namespace ChordFoundry {

//==============================================================================
// Dark Mode Color Palette - Professional Audio Application Design
const juce::Colour ModernLookAndFeel::Colors::primary          = juce::Colour(0xFF00D4FF);  // Bright cyan accent
const juce::Colour ModernLookAndFeel::Colors::primaryDark      = juce::Colour(0xFF00B8E6);  // Darker cyan
const juce::Colour ModernLookAndFeel::Colors::primaryLight     = juce::Colour(0xFF33DDFF);  // Lighter cyan

const juce::Colour ModernLookAndFeel::Colors::secondary        = juce::Colour(0xFF00FF88);  // Bright green accent
const juce::Colour ModernLookAndFeel::Colors::secondaryDark    = juce::Colour(0xFF00E676);  // Darker green
const juce::Colour ModernLookAndFeel::Colors::accent           = juce::Colour(0xFFFF4081);  // Bright pink/red accent

// Dark background hierarchy
const juce::Colour ModernLookAndFeel::Colors::background       = juce::Colour(0xFF0A0A0B);  // Deep dark background
const juce::Colour ModernLookAndFeel::Colors::surface          = juce::Colour(0xFF1A1A1C);  // Panel surfaces
const juce::Colour ModernLookAndFeel::Colors::surfaceElevated  = juce::Colour(0xFF252528);  // Elevated elements

// High contrast text for dark mode
const juce::Colour ModernLookAndFeel::Colors::textPrimary      = juce::Colour(0xFFE8E8E9);  // High contrast white
const juce::Colour ModernLookAndFeel::Colors::textSecondary    = juce::Colour(0xFFA0A0A3);  // Medium contrast gray
const juce::Colour ModernLookAndFeel::Colors::textOnPrimary    = juce::Colour(0xFF0A0A0B);  // Dark text on bright backgrounds

// Professional borders and outlines
const juce::Colour ModernLookAndFeel::Colors::border           = juce::Colour(0xFF3A3A3D);  // Subtle dark borders
const juce::Colour ModernLookAndFeel::Colors::borderFocus      = juce::Colour(0xFF00D4FF);  // Cyan focus indicator
const juce::Colour ModernLookAndFeel::Colors::borderError      = juce::Colour(0xFFFF4444);  // Red error indicator

// Interactive states
const juce::Colour ModernLookAndFeel::Colors::hover            = juce::Colour(0xFF2D2D30);  // Subtle hover state
const juce::Colour ModernLookAndFeel::Colors::pressed          = juce::Colour(0xFF404043);  // Pressed state
const juce::Colour ModernLookAndFeel::Colors::disabled         = juce::Colour(0xFF5A5A5D);  // Disabled elements

// Semantic colors for audio application
const juce::Colour ModernLookAndFeel::Colors::success          = juce::Colour(0xFF00FF88);  // Bright success green
const juce::Colour ModernLookAndFeel::Colors::warning          = juce::Colour(0xFFFFB347);  // Warm warning orange
const juce::Colour ModernLookAndFeel::Colors::error            = juce::Colour(0xFFFF4444);  // Bright error red
const juce::Colour ModernLookAndFeel::Colors::info             = juce::Colour(0xFF00D4FF);  // Info cyan

//==============================================================================
// Typography - Modern, sleek font system
const juce::Font ModernLookAndFeel::Typography::getHeaderFont()
{
    // Scale font size based on display scale factor for better cross-platform support
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 32.0f;  // Slightly larger for modern look
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    // Create modern font with specific typeface
    auto font = juce::Font("Helvetica Neue", adjustedSize, juce::Font::plain);
    
    // If Helvetica Neue not available, try modern alternatives
    if (!font.getTypefaceName().contains("Helvetica"))
    {
        font = juce::Font("Inter", adjustedSize, juce::Font::plain);
        if (!font.getTypefaceName().contains("Inter"))
        {
            font = juce::Font("Segoe UI", adjustedSize, juce::Font::plain);
        }
    }
    
    // Use lighter weight for modern aesthetic with letter spacing
    font = font.withExtraKerningFactor(0.05f);  // Add slight letter spacing
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getSubheaderFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 22.0f;
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font("Helvetica Neue", adjustedSize, juce::Font::plain);
    if (!font.getTypefaceName().contains("Helvetica"))
    {
        font = juce::Font("Inter", adjustedSize, juce::Font::plain);
    }
    
    font = font.withExtraKerningFactor(0.03f);  // Subtle letter spacing
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getBodyFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 15.0f;  // Slightly smaller for cleaner look
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font("Helvetica Neue", adjustedSize, juce::Font::plain);
    if (!font.getTypefaceName().contains("Helvetica"))
    {
        font = juce::Font(adjustedSize);  // Default system font
    }
    
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getCaptionFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 13.0f;
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font(adjustedSize);
    font = font.withExtraKerningFactor(0.02f);  // Slight letter spacing
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getSmallFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 11.0f;
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font(adjustedSize);
    font = font.withExtraKerningFactor(0.03f);  // More spacing for small text
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getButtonFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 15.0f;  // Slightly smaller for cleaner buttons
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font(adjustedSize);
    // Medium weight for buttons - using kerning instead of bold for modern look
    font = font.withExtraKerningFactor(0.04f);
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getMonospacedFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 14.0f;
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    // Try modern monospace fonts
    auto font = juce::Font("SF Mono", adjustedSize, juce::Font::plain);
    if (!font.getTypefaceName().contains("Mono"))
    {
        font = juce::Font("Consolas", adjustedSize, juce::Font::plain);
        if (!font.getTypefaceName().contains("Consolas"))
        {
            font = juce::Font(juce::Font::getDefaultMonospacedFontName(), adjustedSize, juce::Font::plain);
        }
    }
    
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getDisplayFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 48.0f;  // Large for display elements
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font("Helvetica Neue", adjustedSize, juce::Font::plain);
    if (!font.getTypefaceName().contains("Helvetica"))
    {
        font = juce::Font(adjustedSize);
    }
    
    font = font.withExtraKerningFactor(0.06f);  // Extra spacing for large text
    return font;
}

const juce::Font ModernLookAndFeel::Typography::getLabelFont()
{
    auto scaleFactor = static_cast<float>(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale);
    auto baseFontSize = 12.0f;
    auto adjustedSize = baseFontSize * juce::jmax(0.8f, juce::jmin(1.2f, scaleFactor));
    
    auto font = juce::Font(adjustedSize);
    font = font.withExtraKerningFactor(0.05f);  // Wide spacing for labels
    return font.withHorizontalScale(0.95f);  // Slightly condensed
}

//==============================================================================
ModernLookAndFeel::ModernLookAndFeel()
{
    // Set default colors for common components
    setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
    setColour(juce::DocumentWindow::backgroundColourId, Colors::background);
    
    // Button colors
    setColour(juce::TextButton::buttonColourId, Colors::surface);
    setColour(juce::TextButton::buttonOnColourId, Colors::primary);
    setColour(juce::TextButton::textColourOffId, Colors::textPrimary);
    setColour(juce::TextButton::textColourOnId, Colors::textOnPrimary);
    
    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, Colors::surface);
    setColour(juce::ComboBox::textColourId, Colors::textPrimary);
    setColour(juce::ComboBox::outlineColourId, Colors::border);
    setColour(juce::ComboBox::focusedOutlineColourId, Colors::borderFocus);
    
    // Label colors with modern styling
    setColour(juce::Label::textColourId, Colors::textPrimary);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentWhite);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentWhite);
    setColour(juce::Label::textWhenEditingColourId, Colors::textPrimary);
    
    // Popup menu colors
    setColour(juce::PopupMenu::backgroundColourId, Colors::surface);
    setColour(juce::PopupMenu::textColourId, Colors::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colors::hover);
    
    // Scrollbar colors
    setColour(juce::ScrollBar::backgroundColourId, Colors::background);
    setColour(juce::ScrollBar::thumbColourId, Colors::border);
    setColour(juce::ScrollBar::trackColourId, Colors::background);
}

//==============================================================================
void ModernLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerRadius = static_cast<float>(Metrics::cornerRadius);
    
    juce::Colour baseColour = backgroundColour;
    bool hasGlow = false;
    
    // Determine button state and styling
    if (!button.isEnabled())
    {
        baseColour = Colors::disabled;
    }
    else if (button.getToggleState())
    {
        baseColour = Colors::primary;
        hasGlow = true;
    }
    else if (shouldDrawButtonAsDown)
    {
        baseColour = Colors::pressed;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = Colors::hover;
        hasGlow = true;
    }
    else
    {
        baseColour = Colors::surface;
    }
    
    // Use modern button drawing with gradients and glow effects
    drawModernButton(g, bounds, baseColour, shouldDrawButtonAsDown, hasGlow, cornerRadius);
    
    // Enhanced focus indicator
    if (button.hasKeyboardFocus(true))
    {
        drawGlowEffect(g, bounds, Colors::borderFocus, Metrics::glowRadius * 0.8f, cornerRadius);
    }
}

void ModernLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool shouldDrawButtonAsHighlighted,
                                     bool shouldDrawButtonAsDown)
{
    auto font = Typography::getButtonFont();
    g.setFont(font);
    
    // Enable high-quality text rendering
    g.setColour(juce::Colours::transparentBlack);
    g.saveState();
    
    juce::Colour textColour = Colors::textPrimary;
    
    if (button.getToggleState())
    {
        textColour = Colors::textOnPrimary;
    }
    else if (!button.isEnabled())
    {
        textColour = Colors::disabled.brighter(0.2f);  // Slightly brighter for better visibility
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        textColour = Colors::textPrimary.brighter(0.1f);  // Subtle highlight
    }
    
    g.setColour(textColour);
    
    // Modern centered text with better spacing
    auto bounds = button.getLocalBounds().reduced(8, 4);
    
    // Convert to uppercase for modern button aesthetic (optional)
    auto buttonText = button.getButtonText();
    if (button.getHeight() < 30)  // Only uppercase smaller buttons
    {
        buttonText = buttonText.toUpperCase();
    }
    
    g.drawFittedText(buttonText, bounds, juce::Justification::centred, 1);
    
    g.restoreState();
}

void ModernLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                   bool isButtonDown, int buttonX, int buttonY,
                                   int buttonW, int buttonH, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
    auto cornerRadius = static_cast<float>(Metrics::cornerRadius);
    
    juce::Colour baseColour = Colors::surface;
    bool hasGlow = false;
    
    // Determine combo box state
    if (!box.isEnabled())
    {
        baseColour = Colors::disabled;
    }
    else if (isButtonDown)
    {
        baseColour = Colors::pressed;
    }
    else if (box.isMouseOver() || box.hasKeyboardFocus(true))
    {
        baseColour = Colors::hover;
        hasGlow = true;
    }
    
    // Draw modern combo box background with gradient
    drawModernButton(g, bounds, baseColour, isButtonDown, hasGlow, cornerRadius);
    
    // Enhanced focus indicator
    if (box.hasKeyboardFocus(true))
    {
        drawGlowEffect(g, bounds, Colors::borderFocus, Metrics::glowRadius * 0.6f, cornerRadius);
    }
    
    // Draw modern dropdown arrow with enhanced styling
    auto arrowZone = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
    auto centerX = arrowZone.getCentreX();
    auto centerY = arrowZone.getCentreY();
    
    // Create sleek, slightly larger arrow design
    juce::Path arrow;
    arrow.addTriangle(centerX - 5.0f, centerY - 2.5f,
                     centerX + 5.0f, centerY - 2.5f,
                     centerX, centerY + 3.5f);
    
    // Enhanced arrow color logic for better visibility
    juce::Colour arrowColour = Colors::textSecondary;
    if (!box.isEnabled())
    {
        arrowColour = Colors::disabled;
    }
    else if (box.isMouseOver() || box.hasKeyboardFocus(true))
    {
        arrowColour = Colors::primary;
    }
    
    // Draw main arrow
    g.setColour(arrowColour);
    g.fillPath(arrow);
    
    // Add enhanced glow effect for better visibility
    if ((box.isMouseOver() || box.hasKeyboardFocus(true)) && box.isEnabled())
    {
        // Outer glow
        g.setColour(arrowColour.withAlpha(0.2f));
        g.fillPath(arrow, juce::AffineTransform::scale(1.4f, 1.4f, centerX, centerY));
        
        // Inner highlight
        g.setColour(arrowColour.withAlpha(0.4f));
        g.fillPath(arrow, juce::AffineTransform::scale(1.1f, 1.1f, centerX, centerY));
    }
}

void ModernLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    drawRoundedRectWithShadow(g, bounds, Colors::surface, Colors::border, 
                             static_cast<float>(Metrics::cornerRadius), true);
}

void ModernLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                                    int x, int y, int width, int height,
                                    bool isScrollbarVertical, int thumbStartPosition,
                                    int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(x, y, isMouseDown);
    
    auto bounds = juce::Rectangle<int>(0, 0, width, height);
    
    // Draw track
    g.setColour(Colors::background);
    g.fillRect(bounds);
    
    // Draw thumb
    juce::Rectangle<int> thumbBounds;
    if (isScrollbarVertical)
        thumbBounds = juce::Rectangle<int>(0, thumbStartPosition, width, thumbSize);
    else
        thumbBounds = juce::Rectangle<int>(thumbStartPosition, 0, thumbSize, height);
    
    auto cornerRadius = static_cast<float>(juce::jmin(width, height)) / 2.0f;
    
    juce::Colour thumbColour = isMouseOver ? Colors::textSecondary : Colors::border;
    g.setColour(thumbColour);
    g.fillRoundedRectangle(thumbBounds.toFloat(), cornerRadius);
}

void ModernLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRect(slider.isHorizontal() ? juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y) + 0.5f, sliderPos - static_cast<float>(x), static_cast<float>(height) - 1.0f)
                                        : juce::Rectangle<float>(static_cast<float>(x) + 0.5f, sliderPos, static_cast<float>(width) - 1.0f, static_cast<float>(y) + (static_cast<float>(height) - sliderPos)));
    }
    else
    {
        auto isTwoVal = (style == juce::Slider::SliderStyle::TwoValueVertical || style == juce::Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);
        
        auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? static_cast<float>(height) * 0.25f : static_cast<float>(width) * 0.25f);
        
        juce::Point<float> startPoint(slider.isHorizontal() ? static_cast<float>(x) : static_cast<float>(x) + static_cast<float>(width) * 0.5f,
                                     slider.isHorizontal() ? static_cast<float>(y) + static_cast<float>(height) * 0.5f : static_cast<float>(height) + static_cast<float>(y));
        
        juce::Point<float> endPoint(slider.isHorizontal() ? static_cast<float>(width) + static_cast<float>(x) : startPoint.x,
                                   slider.isHorizontal() ? startPoint.y : static_cast<float>(y));
        
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);
        g.setColour(Colors::border);
        g.strokePath(backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        
        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint, thumbPoint;
        
        if (isTwoVal || isThreeVal)
        {
            minPoint = { slider.isHorizontal() ? minSliderPos : static_cast<float>(width) * 0.5f + static_cast<float>(x),
                        slider.isHorizontal() ? static_cast<float>(height) * 0.5f + static_cast<float>(y) : minSliderPos };
            
            maxPoint = { slider.isHorizontal() ? maxSliderPos : static_cast<float>(width) * 0.5f + static_cast<float>(x),
                        slider.isHorizontal() ? static_cast<float>(height) * 0.5f + static_cast<float>(y) : maxSliderPos };
        }
        else
        {
            auto kx = slider.isHorizontal() ? sliderPos : (static_cast<float>(x) + static_cast<float>(width) * 0.5f);
            auto ky = slider.isHorizontal() ? (static_cast<float>(y) + static_cast<float>(height) * 0.5f) : sliderPos;
            
            minPoint = startPoint;
            maxPoint = { kx, ky };
        }
        
        auto thumbWidth = getSliderThumbRadius(slider);
        
        valueTrack.startNewSubPath(minPoint);
        valueTrack.lineTo(isThreeVal ? thumbPoint : maxPoint);
        g.setColour(Colors::primary);
        g.strokePath(valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        
        if (!isTwoVal)
        {
            g.setColour(Colors::primary);
            g.fillEllipse(juce::Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(thumbWidth)).withCentre(isThreeVal ? thumbPoint : maxPoint));
        }
        
        if (isTwoVal || isThreeVal)
        {
            auto sr = juce::jmin(trackWidth, static_cast<float>(thumbWidth));
            g.setColour(Colors::primary);
            g.fillEllipse(juce::Rectangle<float>(sr, sr).withCentre(minPoint));
            g.fillEllipse(juce::Rectangle<float>(sr, sr).withCentre(maxPoint));
        }
    }
}

void ModernLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto fontSize = juce::jmin(15.0f, static_cast<float>(button.getHeight()) * 0.75f);
    auto tickWidth = fontSize * 1.1f;
    
    drawTickBox(g, button, 4.0f, (static_cast<float>(button.getHeight()) - tickWidth) * 0.5f,
               tickWidth, tickWidth, button.getToggleState(),
               button.isEnabled(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    g.setColour(button.isEnabled() ? Colors::textPrimary : Colors::disabled);
    g.setFont(Typography::getBodyFont());
    
    if (!button.isEnabled())
        g.setOpacity(0.5f);
    
    g.drawFittedText(button.getButtonText(),
                    button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
                                          .withTrimmedRight(2),
                    juce::Justification::centredLeft, 10);
}

// Note: Removed drawTabButton implementation due to JUCE version compatibility

void ModernLookAndFeel::drawFocusRectangle(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         juce::Component& component)
{
    juce::ignoreUnused(component);
    
    g.setColour(Colors::borderFocus);
    g.drawRoundedRectangle(area.toFloat().expanded(2.0f), 
                          static_cast<float>(Metrics::cornerRadius), 
                          static_cast<float>(Metrics::focusOutlineWidth));
}

//==============================================================================
void ModernLookAndFeel::drawRoundedRectWithShadow(juce::Graphics& g,
                                                 const juce::Rectangle<float>& bounds,
                                                 const juce::Colour& fillColour,
                                                 const juce::Colour& borderColour,
                                                 float cornerRadius,
                                                 bool drawShadow)
{
    if (drawShadow)
    {
        // Draw shadow
        auto shadowBounds = bounds.translated(0, static_cast<float>(Metrics::shadowOffset));
        g.setColour(juce::Colours::black.withAlpha(Metrics::shadowOpacity));
        g.fillRoundedRectangle(shadowBounds, cornerRadius);
    }
    
    // Draw main shape
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Draw border
    if (borderColour != juce::Colours::transparentBlack)
    {
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds, cornerRadius, static_cast<float>(Metrics::borderWidth));
    }
}

void ModernLookAndFeel::drawCard(juce::Graphics& g, const juce::Rectangle<int>& bounds, bool isElevated)
{
    auto cardBounds = bounds.toFloat();
    auto cornerRadius = static_cast<float>(Metrics::cornerRadius);
    
    if (isElevated)
    {
        // Create subtle gradient for elevated cards
        drawGradientBackground(g, cardBounds, 
                             Colors::surfaceElevated.brighter(0.05f),
                             Colors::surfaceElevated.darker(0.05f),
                             cornerRadius);
        
        // Add subtle glow effect
        drawGlowEffect(g, cardBounds, Colors::primary.withAlpha(0.1f), 3.0f, cornerRadius);
    }
    else
    {
        // Standard card with subtle gradient
        drawGradientBackground(g, cardBounds,
                             Colors::surface.brighter(0.02f),
                             Colors::surface.darker(0.02f),
                             cornerRadius);
    }
    
    // Draw border
    g.setColour(Colors::border);
    g.drawRoundedRectangle(cardBounds, cornerRadius, static_cast<float>(Metrics::borderWidth));
}

void ModernLookAndFeel::drawGradientBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                             const juce::Colour& topColour, const juce::Colour& bottomColour,
                                             float cornerRadius)
{
    juce::ColourGradient gradient(topColour, bounds.getTopLeft(),
                                 bottomColour, bounds.getBottomLeft(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

void ModernLookAndFeel::drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                      const juce::Colour& glowColour, float radius,
                                      float cornerRadius)
{
    // Create multiple layers for a smooth glow effect
    for (int i = static_cast<int>(radius); i > 0; --i)
    {
        float alpha = (1.0f - (static_cast<float>(i) / radius)) * glowColour.getFloatAlpha();
        auto expandedBounds = bounds.expanded(static_cast<float>(i));
        
        g.setColour(glowColour.withAlpha(alpha * 0.3f));
        g.drawRoundedRectangle(expandedBounds, cornerRadius + static_cast<float>(i), 1.0f);
    }
}

void ModernLookAndFeel::drawModernButton(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                       const juce::Colour& baseColour, bool isPressed,
                                       bool hasGlow, float cornerRadius)
{
    auto buttonBounds = bounds;
    
    if (isPressed)
    {
        // Pressed state: darker gradient, slightly inset
        buttonBounds = bounds.reduced(1.0f);
        drawGradientBackground(g, buttonBounds,
                             baseColour.darker(0.2f),
                             baseColour.darker(0.1f),
                             cornerRadius);
    }
    else
    {
        // Normal state: subtle gradient
        drawGradientBackground(g, buttonBounds,
                             baseColour.brighter(0.1f),
                             baseColour.darker(0.1f),
                             cornerRadius);
        
        if (hasGlow)
        {
            drawGlowEffect(g, buttonBounds, baseColour.withAlpha(0.5f), 
                          Metrics::glowRadius, cornerRadius);
        }
    }
    
    // Draw border
    g.setColour(baseColour.contrasting(0.3f));
    g.drawRoundedRectangle(buttonBounds, cornerRadius, static_cast<float>(Metrics::borderWidth));
}

juce::Colour ModernLookAndFeel::getChordColour(const juce::String& romanNumeral)
{
    // Color coding based on chord function
    if (romanNumeral.contains("I") || romanNumeral.contains("IV") || romanNumeral.contains("V"))
        return Colors::primary;        // Primary chords (I, IV, V)
    else if (romanNumeral.contains("ii") || romanNumeral.contains("iii") || romanNumeral.contains("vi"))
        return Colors::secondary;      // Secondary chords (ii, iii, vi)
    else if (romanNumeral.contains("°") || romanNumeral.contains("vii"))
        return Colors::accent;         // Diminished chords
    else
        return Colors::textSecondary;  // Default/other chords
}

} // namespace ChordFoundry