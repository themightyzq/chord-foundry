#include "ChordPanelComponent.h"
#include <cmath>

namespace ChordFoundry {

//==============================================================================
ChordPanelComponent::ChordPanelComponent()
{
    setupChordData();
    setupUI();
    
    // Set accessibility properties
    setAccessible(true);
    setTitle("Chord Selection Panel");
    setDescription("Select chords by clicking on the pie slices in the wheel");
}

//==============================================================================
void ChordPanelComponent::paint(juce::Graphics& g)
{
    // Draw card background
    ModernLookAndFeel::drawCard(g, getLocalBounds(), true);
    
    // Calculate wheel bounds
    auto wheelBounds = getLocalBounds().withSizeKeepingCentre(WHEEL_SIZE, WHEEL_SIZE)
                                      .translated(0, -20); // Offset for header
    auto center = wheelBounds.getCentre().toFloat();
    
    float outerRadius = WHEEL_SIZE * 0.5f;
    float innerRadius = CENTER_CIRCLE_SIZE * 0.5f;
    
    // Draw dark background circle to ensure clean rendering
    g.setColour(ModernLookAndFeel::Colors::background);
    g.fillEllipse(wheelBounds.toFloat());
    
    // Draw pie slices
    float anglePerSlice = (2.0f * juce::MathConstants<float>::pi) / static_cast<float>(chordData.size());
    float startAngle = -juce::MathConstants<float>::halfPi; // Start from top
    
    // Debug output removed for production
    
    for (size_t i = 0; i < chordData.size(); ++i)
    {
        float endAngle = startAngle + anglePerSlice;
        drawPieSlice(g, static_cast<int>(i), startAngle, endAngle, innerRadius, outerRadius, center);
        startAngle = endAngle;
    }
    
    // Draw center circle with gradient for depth
    auto centerBounds = wheelBounds.withSizeKeepingCentre(CENTER_CIRCLE_SIZE, CENTER_CIRCLE_SIZE);
    
    juce::ColourGradient centerGradient(
        ModernLookAndFeel::Colors::surfaceElevated.brighter(0.1f), center,
        ModernLookAndFeel::Colors::surfaceElevated.darker(0.1f), 
        center.translated(CENTER_CIRCLE_SIZE * 0.3f, CENTER_CIRCLE_SIZE * 0.3f), true);
    
    g.setGradientFill(centerGradient);
    g.fillEllipse(centerBounds.toFloat());
    
    // Draw center circle border
    g.setColour(ModernLookAndFeel::Colors::border.brighter(0.2f));
    g.drawEllipse(centerBounds.toFloat(), 2.0f);
    
    // Draw selected chord info in center with modern typography
    if (!selectedRoman.isEmpty())
    {
        // Use display font for the main chord symbol
        g.setColour(ModernLookAndFeel::Colors::primary.brighter(0.2f));
        g.setFont(ModernLookAndFeel::Typography::getDisplayFont().withHeight(36.0f));
        
        auto chordBounds = centerBounds.reduced(0, 10);
        g.drawText(selectedRoman, chordBounds, juce::Justification::centred);
        
        // Draw chord name below with modern label font
        auto chordName = getChordFullName(selectedRoman);
        if (!chordName.isEmpty())
        {
            g.setColour(ModernLookAndFeel::Colors::textSecondary.brighter(0.3f));
            g.setFont(ModernLookAndFeel::Typography::getLabelFont());
            auto nameBounds = centerBounds.removeFromBottom(25);
            g.drawText(chordName.toUpperCase(), nameBounds, juce::Justification::centred);
        }
    }
    else
    {
        // Placeholder text with modern styling
        g.setColour(ModernLookAndFeel::Colors::textSecondary.withAlpha(0.6f));
        g.setFont(ModernLookAndFeel::Typography::getCaptionFont());
        g.drawText("SELECT CHORD", centerBounds, juce::Justification::centred);
    }
}

void ChordPanelComponent::resized()
{
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    
    // Header area
    auto headerBounds = bounds.removeFromTop(60);
    if (headerLabel)
        headerLabel->setBounds(headerBounds);
    
    // Button area at bottom
    auto buttonArea = bounds.removeFromBottom(ModernLookAndFeel::Metrics::buttonHeight + ModernLookAndFeel::Metrics::spacingMD);
    auto buttonWidth = (buttonArea.getWidth() - ModernLookAndFeel::Metrics::spacingSM) / 2;
    
    if (addButton)
    {
        addButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingSM);
    }
    
    if (randomizeButton)
        randomizeButton->setBounds(buttonArea);
    
    // Pie slices are drawn in paint() method, no need to arrange buttons
}

//==============================================================================
void ChordPanelComponent::buttonClicked(juce::Button* button)
{
    // Only handle Add and Randomize buttons now
    if (button == addButton.get())
    {
        if (onAddChord)
            onAddChord();
    }
    else if (button == randomizeButton.get())
    {
        // Select random chord
        auto randomIndex = juce::Random::getSystemRandom().nextInt(static_cast<int>(chordData.size()));
        selectedSliceIndex = randomIndex;
        auto randomChord = chordData[static_cast<size_t>(randomIndex)].roman;
        setSelectedChord(randomChord);
        
        if (onChordSelected)
            onChordSelected(randomChord);
    }
}

void ChordPanelComponent::setSelectedChord(const juce::String& roman)
{
    if (selectedRoman != roman)
    {
        selectedRoman = roman;
        updateChordSelection();
        repaint();
    }
}

//==============================================================================
void ChordPanelComponent::setupChordData()
{
    // Define the chord data with distinct colors for pie slices
    chordData = {
        {"I",    "Major",          ModernLookAndFeel::Colors::primary.darker(0.2f),     true},
        {"ii",   "Minor",          ModernLookAndFeel::Colors::secondary.darker(0.1f),   false},
        {"iii",  "Minor",          ModernLookAndFeel::Colors::secondary.darker(0.3f),   false},
        {"IV",   "Major",          ModernLookAndFeel::Colors::primary.darker(0.1f),     true},
        {"V",    "Major",          ModernLookAndFeel::Colors::primary,                  true},
        {"vi",   "Minor",          ModernLookAndFeel::Colors::secondary,                false},
        {"vii°", "Diminished",     ModernLookAndFeel::Colors::accent.darker(0.2f),      false}
    };
}

void ChordPanelComponent::setupUI()
{
    // Header label
    headerLabel = std::make_unique<juce::Label>("header", "Chord Selection");
    headerLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    headerLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    headerLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*headerLabel);
    
    // No longer creating individual chord buttons - using pie slices instead
    // Enable mouse hover for pie slices
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    // Add chord button
    addButton = std::make_unique<juce::TextButton>("Add Chord");
    addButton->addListener(this);
    addButton->setEnabled(false); // Enabled when chord is selected
    addButton->setAccessible(true);
    addButton->setTitle("Add selected chord to progression");
    addAndMakeVisible(*addButton);
    
    // Randomize button
    randomizeButton = std::make_unique<juce::TextButton>("Random");
    randomizeButton->addListener(this);
    randomizeButton->setAccessible(true);
    randomizeButton->setTitle("Select a random chord");
    addAndMakeVisible(*randomizeButton);
}

void ChordPanelComponent::updateChordSelection()
{
    // Update selected slice index based on selected roman numeral
    selectedSliceIndex = -1;
    for (size_t i = 0; i < chordData.size(); ++i)
    {
        if (chordData[i].roman == selectedRoman)
        {
            selectedSliceIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Enable/disable add button
    if (addButton)
        addButton->setEnabled(!selectedRoman.isEmpty());
}

void ChordPanelComponent::drawPieSlice(juce::Graphics& g, int sliceIndex, float startAngle, float endAngle,
                                     float innerRadius, float outerRadius, const juce::Point<float>& center)
{
    auto& chord = chordData[static_cast<size_t>(sliceIndex)];
    
    // Create pie slice path
    auto slicePath = createPieSlicePath(startAngle, endAngle, innerRadius, outerRadius, center);
    
    // Determine colors based on state
    auto baseColour = chord.colour;
    bool isHovered = (hoveredSliceIndex == sliceIndex);
    bool isSelected = (selectedSliceIndex == sliceIndex);
    
    if (isSelected)
    {
        // Selected state - brighter without overlapping glow
        auto fillColour = baseColour.brighter(0.4f);
        
        // Fill with gradient
        juce::ColourGradient sliceGradient(
            fillColour, center,
            fillColour.darker(0.1f), center.translated(outerRadius, 0), true);
        g.setGradientFill(sliceGradient);
        g.fillPath(slicePath);
    }
    else if (isHovered)
    {
        // Hover state - slightly brighter
        auto fillColour = baseColour.brighter(0.15f);
        
        juce::ColourGradient sliceGradient(
            fillColour, center,
            fillColour.darker(0.15f), center.translated(outerRadius, 0), true);
        g.setGradientFill(sliceGradient);
        g.fillPath(slicePath);
    }
    else
    {
        // Normal state - no transparency
        juce::ColourGradient sliceGradient(
            baseColour, center,
            baseColour.darker(0.2f), center.translated(outerRadius, 0), true);
        g.setGradientFill(sliceGradient);
        g.fillPath(slicePath);
    }
    
    // Draw borders after filling to ensure clean edges
    g.setColour(ModernLookAndFeel::Colors::background.darker(0.8f));
    g.strokePath(slicePath, juce::PathStrokeType(0.5f));
    
    // Draw chord text
    float midAngle = (startAngle + endAngle) * 0.5f;
    float textRadius = (innerRadius + outerRadius) * 0.5f;
    
    auto textPos = center.translated(
        textRadius * std::cos(midAngle),
        textRadius * std::sin(midAngle)
    );
    
    // Draw text with better contrast
    auto textColour = ModernLookAndFeel::Colors::textPrimary;
    if (isSelected || isHovered)
    {
        textColour = textColour.brighter(0.3f);
    }
    
    // Add text shadow for better readability
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    auto shadowBounds = juce::Rectangle<float>(50, 25).withCentre(textPos.translated(1, 1));
    g.drawText(chord.roman, shadowBounds, juce::Justification::centred);
    
    // Draw main text
    g.setColour(textColour);
    g.setFont(ModernLookAndFeel::Typography::getButtonFont().withHeight(16.0f));
    
    auto textBounds = juce::Rectangle<float>(50, 25).withCentre(textPos);
    g.drawText(chord.roman, textBounds, juce::Justification::centred);
}

juce::String ChordPanelComponent::getChordFullName(const juce::String& roman) const
{
    for (const auto& chord : chordData)
    {
        if (chord.roman == roman)
            return chord.fullName;
    }
    return {};
}

juce::Path ChordPanelComponent::createPieSlicePath(float startAngle, float endAngle, 
                                                 float innerRadius, float outerRadius,
                                                 const juce::Point<float>& center) const
{
    juce::Path path;
    
    // Create pie slice using proper arc segments
    const int numSegments = 20; // More segments for smoother curves
    
    // Start at inner radius
    path.startNewSubPath(center.x + innerRadius * std::cos(startAngle),
                        center.y + innerRadius * std::sin(startAngle));
    
    // Line to outer radius
    path.lineTo(center.x + outerRadius * std::cos(startAngle),
               center.y + outerRadius * std::sin(startAngle));
    
    // Draw outer arc manually with segments
    float angleStep = (endAngle - startAngle) / numSegments;
    for (int i = 1; i <= numSegments; ++i)
    {
        float angle = startAngle + angleStep * i;
        path.lineTo(center.x + outerRadius * std::cos(angle),
                   center.y + outerRadius * std::sin(angle));
    }
    
    // Line back to inner radius
    path.lineTo(center.x + innerRadius * std::cos(endAngle),
               center.y + innerRadius * std::sin(endAngle));
    
    // Draw inner arc (backwards)
    for (int i = numSegments - 1; i >= 0; --i)
    {
        float angle = startAngle + angleStep * i;
        path.lineTo(center.x + innerRadius * std::cos(angle),
                   center.y + innerRadius * std::sin(angle));
    }
    
    path.closeSubPath();
    return path;
}

int ChordPanelComponent::getSliceIndexAt(const juce::Point<int>& point) const
{
    auto wheelBounds = getLocalBounds().withSizeKeepingCentre(WHEEL_SIZE, WHEEL_SIZE)
                                      .translated(0, -20);
    auto center = wheelBounds.getCentre();
    
    // Check if point is within wheel
    auto distance = point.getDistanceFrom(center);
    float innerRadius = CENTER_CIRCLE_SIZE * 0.5f;
    float outerRadius = WHEEL_SIZE * 0.5f;
    
    if (distance < innerRadius || distance > outerRadius)
        return -1;
    
    // Calculate angle from center
    auto dx = static_cast<float>(point.x - center.x);
    auto dy = static_cast<float>(point.y - center.y);
    auto angle = std::atan2(dy, dx);
    
    // Normalize angle to 0-2π range, starting from top
    angle += juce::MathConstants<float>::halfPi;
    if (angle < 0)
        angle += 2.0f * juce::MathConstants<float>::pi;
    
    // Calculate which slice
    float anglePerSlice = (2.0f * juce::MathConstants<float>::pi) / static_cast<float>(chordData.size());
    int sliceIndex = static_cast<int>(angle / anglePerSlice);
    
    return juce::jlimit(0, static_cast<int>(chordData.size()) - 1, sliceIndex);
}

void ChordPanelComponent::mouseMove(const juce::MouseEvent& event)
{
    int newHoveredIndex = getSliceIndexAt(event.getPosition());
    
    if (newHoveredIndex != hoveredSliceIndex)
    {
        hoveredSliceIndex = newHoveredIndex;
        repaint();
    }
}

void ChordPanelComponent::mouseDown(const juce::MouseEvent& event)
{
    int clickedIndex = getSliceIndexAt(event.getPosition());
    
    if (clickedIndex >= 0)
    {
        selectedSliceIndex = clickedIndex;
        auto roman = chordData[static_cast<size_t>(clickedIndex)].roman;
        setSelectedChord(roman);
        
        if (onChordSelected)
            onChordSelected(roman);
    }
}

void ChordPanelComponent::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    if (hoveredSliceIndex != -1)
    {
        hoveredSliceIndex = -1;
        repaint();
    }
}

} // namespace ChordFoundry