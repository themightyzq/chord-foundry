#include "StructurePanelComponent.h"

namespace ChordFoundry {

//==============================================================================
// ChordButton Implementation
StructurePanelComponent::ChordButton::ChordButton(const ChordData& chord, int index)
    : Button("ChordButton_" + juce::String(index)), chordData(chord), chordIndex(index)
{
    setAccessible(true);
    setTitle(chord.roman + " chord");
    setDescription("Chord " + juce::String(index + 1) + ": " + chord.roman);
}

void StructurePanelComponent::ChordButton::paintButton(juce::Graphics& g, 
                                                      bool shouldDrawButtonAsHighlighted, 
                                                      bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerRadius = ModernLookAndFeel::Metrics::cornerRadius;
    
    // Determine colors based on state
    juce::Colour fillColour = ModernLookAndFeel::Colors::surface;
    juce::Colour borderColour = ModernLookAndFeel::Colors::border;
    juce::Colour textColour = ModernLookAndFeel::Colors::textPrimary;
    
    if (isCurrentChord)
    {
        fillColour = ModernLookAndFeel::Colors::primary;
        borderColour = ModernLookAndFeel::Colors::primaryDark;
        textColour = ModernLookAndFeel::Colors::textOnPrimary;
    }
    else if (shouldDrawButtonAsDown)
    {
        fillColour = ModernLookAndFeel::Colors::pressed;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        fillColour = ModernLookAndFeel::Colors::hover;
    }
    
    // Get chord color for accent
    auto chordColour = ModernLookAndFeel::getChordColour(chordData.roman);
    
    // Draw button background
    ModernLookAndFeel::drawRoundedRectWithShadow(g, bounds, fillColour, borderColour, 
                                                static_cast<float>(cornerRadius), true);
    
    // Draw chord color accent bar
    if (!isCurrentChord)
    {
        auto accentBounds = bounds.removeFromTop(4.0f);
        g.setColour(chordColour);
        g.fillRoundedRectangle(accentBounds, static_cast<float>(cornerRadius / 2));
    }
    
    // Draw Roman numeral
    g.setColour(textColour);
    g.setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    
    auto textBounds = bounds.reduced(4.0f);
    g.drawText(chordData.roman, textBounds.removeFromTop(textBounds.getHeight() * 0.6f), 
               juce::Justification::centred);
    
    // Draw chord extensions/modifications if any
    if (!chordData.extension.isEmpty() || !chordData.inversion.isEmpty())
    {
        g.setFont(ModernLookAndFeel::Typography::getSmallFont());
        juce::String modifiers;
        if (!chordData.extension.isEmpty())
            modifiers += chordData.extension;
        if (!chordData.inversion.isEmpty())
            modifiers += (modifiers.isEmpty() ? "" : " ") + chordData.inversion;
        
        g.drawText(modifiers, textBounds, juce::Justification::centred);
    }
}

//==============================================================================
// StructurePanelComponent Implementation
StructurePanelComponent::StructurePanelComponent()
{
    setupUI();
    
    // Set accessibility properties
    setAccessible(true);
    setTitle("Chord Progression Structure Panel");
    setDescription("Display and manage the current chord progression sequence");
}

//==============================================================================
void StructurePanelComponent::paint(juce::Graphics& g)
{
    // Draw card background
    ModernLookAndFeel::drawCard(g, getLocalBounds(), true);
    
    // If no chords, show helpful message
    if (chords.empty())
    {
        auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingLG);
        bounds.removeFromTop(60); // Header space
        bounds.removeFromBottom(60); // Button space
        
        g.setColour(ModernLookAndFeel::Colors::textSecondary);
        g.setFont(ModernLookAndFeel::Typography::getBodyFont());
        
        juce::String helpText = "No chords in progression\n\n";
        helpText += "Add chords using the Chord Panel\n";
        helpText += "or click 'Randomize' to generate\n";
        helpText += "a progression automatically";
        
        g.drawText(helpText, bounds, juce::Justification::centred);
    }
    
    // Draw progression analysis if chords exist
    else if (chords.size() > 1)
    {
        auto analysisBounds = getLocalBounds().removeFromBottom(40).reduced(ModernLookAndFeel::Metrics::spacingMD);
        
        g.setColour(ModernLookAndFeel::Colors::textSecondary);
        g.setFont(ModernLookAndFeel::Typography::getCaptionFont());
        
        auto analysisText = analyzeProgression();
        g.drawText(analysisText, analysisBounds, juce::Justification::centredLeft);
    }
}

void StructurePanelComponent::resized()
{
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    
    // Header area
    auto headerBounds = bounds.removeFromTop(60);
    if (headerLabel)
        headerLabel->setBounds(headerBounds);
    
    // Button area at bottom
    auto buttonArea = bounds.removeFromBottom(ModernLookAndFeel::Metrics::buttonHeight + ModernLookAndFeel::Metrics::spacingMD);
    auto buttonWidth = (buttonArea.getWidth() - (ModernLookAndFeel::Metrics::spacingSM * 2)) / 3;
    
    if (clearButton)
    {
        clearButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingSM);
    }
    
    if (randomizeButton)
    {
        randomizeButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingSM);
    }
    
    if (exportButton)
        exportButton->setBounds(buttonArea);
    
    // Chord viewport area (remaining space)
    if (chordViewport)
    {
        chordViewport->setBounds(bounds);
        updateChordButtonLayout();
    }
}

//==============================================================================
void StructurePanelComponent::buttonClicked(juce::Button* button)
{
    // Check if it's a chord button
    for (auto& chordButton : chordButtons)
    {
        if (chordButton.get() == button)
        {
            auto index = chordButton->getChordIndex();
            if (onChordSelected)
                onChordSelected(index);
            return;
        }
    }
    
    // Check control buttons
    if (button == clearButton.get())
    {
        if (onClearAll)
            onClearAll();
    }
    else if (button == randomizeButton.get())
    {
        if (onRandomizeProgression)
            onRandomizeProgression();
    }
    else if (button == exportButton.get())
    {
        if (onExportProgression)
            onExportProgression();
    }
}

bool StructurePanelComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Accept chord button drags for reordering
    return dragSourceDetails.description.toString().startsWith("ChordButton_");
}

void StructurePanelComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    // TODO: Implement drag and drop chord reordering
    juce::ignoreUnused(dragSourceDetails);
    DBG("Chord drag and drop - to be implemented");
}

//==============================================================================
void StructurePanelComponent::setChordProgression(const std::vector<ChordData>& newChords)
{
    if (chords != newChords)
    {
        chords = newChords;
        updateChordButtons();
        repaint();
    }
}

void StructurePanelComponent::setCurrentChordIndex(int index)
{
    if (currentChordIndex != index)
    {
        currentChordIndex = index;
        
        // Update chord button states
        for (size_t i = 0; i < chordButtons.size(); ++i)
        {
            chordButtons[i]->setIsCurrentChord(static_cast<int>(i) == index);
        }
    }
}

void StructurePanelComponent::clearProgression()
{
    chords.clear();
    updateChordButtons();
    repaint();
}

void StructurePanelComponent::addChord(const ChordData& chord)
{
    chords.push_back(chord);
    updateChordButtons();
    repaint();
}

void StructurePanelComponent::removeChord(int index)
{
    if (index >= 0 && index < static_cast<int>(chords.size()))
    {
        chords.erase(chords.begin() + index);
        updateChordButtons();
        repaint();
    }
}

void StructurePanelComponent::updateChord(int index, const ChordData& newChord)
{
    if (index >= 0 && index < static_cast<int>(chords.size()))
    {
        chords[static_cast<size_t>(index)] = newChord;
        if (index < static_cast<int>(chordButtons.size()))
        {
            chordButtons[static_cast<size_t>(index)]->setChordData(newChord);
        }
        repaint();
    }
}

//==============================================================================
void StructurePanelComponent::setupUI()
{
    // Header label
    headerLabel = std::make_unique<juce::Label>("header", "Chord Progression");
    headerLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    headerLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    headerLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*headerLabel);
    
    // Clear button
    clearButton = std::make_unique<juce::TextButton>("Clear");
    clearButton->addListener(this);
    clearButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::error);
    clearButton->setAccessible(true);
    clearButton->setTitle("Clear all chords from progression");
    addAndMakeVisible(*clearButton);
    
    // Randomize button
    randomizeButton = std::make_unique<juce::TextButton>("Randomize");
    randomizeButton->addListener(this);
    randomizeButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::secondary);
    randomizeButton->setAccessible(true);
    randomizeButton->setTitle("Generate random chord progression");
    addAndMakeVisible(*randomizeButton);
    
    // Export button
    exportButton = std::make_unique<juce::TextButton>("Export");
    exportButton->addListener(this);
    exportButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::info);
    exportButton->setAccessible(true);
    exportButton->setTitle("Export progression to MIDI or other formats");
    addAndMakeVisible(*exportButton);
    
    // Chord viewport and container
    chordViewport = std::make_unique<juce::Viewport>("chordViewport");
    chordContainer = std::make_unique<juce::Component>("chordContainer");
    
    chordViewport->setViewedComponent(chordContainer.get(), false);
    chordViewport->setScrollBarsShown(true, false);
    addAndMakeVisible(*chordViewport);
}

void StructurePanelComponent::updateChordButtons()
{
    // Clear existing buttons
    chordButtons.clear();
    chordContainer->removeAllChildren();
    
    // Create new buttons for each chord
    for (size_t i = 0; i < chords.size(); ++i)
    {
        auto button = std::make_unique<ChordButton>(chords[i], static_cast<int>(i));
        button->addListener(this);
        chordContainer->addAndMakeVisible(*button);
        chordButtons.push_back(std::move(button));
    }
    
    updateChordButtonLayout();
}

void StructurePanelComponent::updateChordButtonLayout()
{
    if (!chordContainer || chordButtons.empty())
        return;
    
    auto viewportBounds = chordViewport->getViewArea();
    auto availableWidth = viewportBounds.getWidth();
    
    // Calculate buttons per row based on available width
    auto buttonsPerRow = juce::jmax(1, availableWidth / (CHORD_BUTTON_WIDTH + ModernLookAndFeel::Metrics::spacingSM));
    auto rows = (static_cast<int>(chordButtons.size()) + buttonsPerRow - 1) / buttonsPerRow;
    
    // Set container size
    auto containerHeight = rows * (CHORD_BUTTON_HEIGHT + ModernLookAndFeel::Metrics::spacingSM) - ModernLookAndFeel::Metrics::spacingSM;
    chordContainer->setSize(availableWidth, juce::jmax(containerHeight, viewportBounds.getHeight()));
    
    // Position buttons
    for (size_t i = 0; i < chordButtons.size(); ++i)
    {
        auto row = static_cast<int>(i) / buttonsPerRow;
        auto col = static_cast<int>(i) % buttonsPerRow;
        
        auto x = col * (CHORD_BUTTON_WIDTH + ModernLookAndFeel::Metrics::spacingSM);
        auto y = row * (CHORD_BUTTON_HEIGHT + ModernLookAndFeel::Metrics::spacingSM);
        
        chordButtons[i]->setBounds(x, y, CHORD_BUTTON_WIDTH, CHORD_BUTTON_HEIGHT);
        chordButtons[i]->setChordIndex(static_cast<int>(i));
    }
}

juce::String StructurePanelComponent::analyzeProgression() const
{
    if (chords.size() < 2)
        return "Add more chords for analysis";
    
    juce::String analysis = "Progression: ";
    for (size_t i = 0; i < chords.size(); ++i)
    {
        analysis += chords[i].roman;
        if (i < chords.size() - 1)
            analysis += " - ";
    }
    
    // Simple analysis based on common progressions
    juce::String progressionString;
    for (const auto& chord : chords)
        progressionString += chord.roman + "-";
    
    if (progressionString.contains("I-V-vi-IV"))
        analysis += " | Pop progression (vi-IV-I-V)";
    else if (progressionString.contains("ii-V-I"))
        analysis += " | Jazz ii-V-I cadence";
    else if (progressionString.contains("I-vi-IV-V"))
        analysis += " | Classic I-vi-IV-V";
    else if (progressionString.contains("vi-IV-I-V"))
        analysis += " | Pop ballad progression";
    
    return analysis;
}

juce::String StructurePanelComponent::getChordFunctionAnalysis(const ChordData& chord, int index) const
{
    juce::ignoreUnused(index);
    
    // Basic function analysis
    if (chord.roman == "I")
        return "Tonic - Home";
    else if (chord.roman == "V" || chord.roman == "V7")
        return "Dominant - Tension";
    else if (chord.roman == "IV")
        return "Subdominant - Departure";
    else if (chord.roman == "vi")
        return "Relative minor";
    else if (chord.roman == "ii")
        return "Predominant";
    else
        return "";
}

} // namespace ChordFoundry