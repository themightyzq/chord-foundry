#include "PatternEditorComponent.h"

namespace ChordFoundry {

//==============================================================================
// StepButton Implementation
PatternEditorComponent::StepButton::StepButton(int stepNumber)
    : Button("Step_" + juce::String(stepNumber)), stepNumber(stepNumber)
{
    setAccessible(true);
    setTitle("Step " + juce::String(stepNumber + 1));
    setDescription("Pattern step " + juce::String(stepNumber + 1) + " of 32");
}

void PatternEditorComponent::StepButton::paintButton(juce::Graphics& g, 
                                                    bool shouldDrawButtonAsHighlighted, 
                                                    bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerRadius = 4.0f;
    
    // Determine colors based on state
    juce::Colour fillColour = ModernLookAndFeel::Colors::background;
    juce::Colour borderColour = ModernLookAndFeel::Colors::border;
    
    if (hasBlock)
    {
        fillColour = blockColour;
        borderColour = blockColour.darker(0.2f);
    }
    else if (shouldDrawButtonAsDown)
    {
        fillColour = ModernLookAndFeel::Colors::pressed;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        fillColour = ModernLookAndFeel::Colors::hover;
    }
    
    // Draw step background
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Draw border
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    
    // Draw current step indicator (playhead)
    if (isCurrentStep)
    {
        g.setColour(ModernLookAndFeel::Colors::warning);
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerRadius, 2.0f);
        
        // Draw play indicator
        auto centerBounds = bounds.withSizeKeepingCentre(8.0f, 8.0f);
        g.fillEllipse(centerBounds);
    }
    
    // Draw step number for every 4th step
    if ((stepNumber % 4) == 0)
    {
        g.setColour(hasBlock ? ModernLookAndFeel::Colors::textOnPrimary 
                             : ModernLookAndFeel::Colors::textSecondary);
        g.setFont(ModernLookAndFeel::Typography::getSmallFont());
        g.drawText(juce::String(stepNumber + 1), bounds.reduced(2.0f), 
                  juce::Justification::centred);
    }
}

//==============================================================================
// PatternEditorComponent Implementation
PatternEditorComponent::PatternEditorComponent()
{
    setupUI();
    setupStepButtons();
    
    // Set accessibility properties
    setAccessible(true);
    setTitle("Pattern Editor - 32-Step Sequencer");
    setDescription("Edit pattern blocks by clicking and dragging on the 32-step grid");
}

//==============================================================================
void PatternEditorComponent::paint(juce::Graphics& g)
{
    // Draw card background
    ModernLookAndFeel::drawCard(g, getLocalBounds(), true);
    
    // Draw grid background
    auto gridArea = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    gridArea.removeFromTop(60); // Header
    gridArea.removeFromBottom(60); // Buttons
    
    // Draw step numbers and beat indicators
    drawStepNumbers(g, gridArea);
    
    // Draw playhead if playing
    if (currentStep >= 0)
    {
        drawPlayhead(g, gridArea);
    }
    
    // Show helpful message if no blocks
    if (blocks.empty())
    {
        auto textArea = gridArea.reduced(ModernLookAndFeel::Metrics::spacingLG);
        
        g.setColour(ModernLookAndFeel::Colors::textSecondary);
        g.setFont(ModernLookAndFeel::Typography::getBodyFont());
        
        juce::String helpText = "Click and drag to create pattern blocks\\n\\n";
        helpText += "• Each block represents when a chord plays\\n";
        helpText += "• Drag to adjust block length\\n"; 
        helpText += "• Right-click to remove blocks";
        
        g.drawText(helpText, textArea, juce::Justification::centred);
    }
}

void PatternEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    
    // Header area
    auto headerBounds = bounds.removeFromTop(60);
    if (headerLabel)
        headerLabel->setBounds(headerBounds);
    
    // Button area at bottom
    auto buttonArea = bounds.removeFromBottom(ModernLookAndFeel::Metrics::buttonHeight + ModernLookAndFeel::Metrics::spacingMD);
    auto buttonWidth = (buttonArea.getWidth() - ModernLookAndFeel::Metrics::spacingSM) / 2;
    
    if (clearButton)
    {
        clearButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingSM);
    }
    
    if (randomizeButton)
        randomizeButton->setBounds(buttonArea);
    
    // Step buttons grid (remaining space)
    auto gridArea = bounds;
    auto stepButtonWidth = (gridArea.getWidth() - ((STEPS_PER_ROW - 1) * GRID_SPACING)) / STEPS_PER_ROW;
    auto buttonHeight = (gridArea.getHeight() - GRID_SPACING) / 2; // 2 rows
    
    // Position step buttons in a 16x2 grid
    for (int i = 0; i < TOTAL_STEPS; ++i)
    {
        if (i < static_cast<int>(stepButtons.size()))
        {
            int row = i / STEPS_PER_ROW;
            int col = i % STEPS_PER_ROW;
            
            int x = gridArea.getX() + col * (stepButtonWidth + GRID_SPACING);
            int y = gridArea.getY() + row * (buttonHeight + GRID_SPACING);
            
            stepButtons[static_cast<size_t>(i)]->setBounds(x, y, stepButtonWidth, buttonHeight);
        }
    }
}

//==============================================================================
void PatternEditorComponent::buttonClicked(juce::Button* button)
{
    // Check if it's a step button
    for (size_t i = 0; i < stepButtons.size(); ++i)
    {
        if (stepButtons[i].get() == button)
        {
            int stepNum = static_cast<int>(i);
            
            // Toggle block at this step
            if (findBlockAtStep(stepNum))
            {
                removeBlockAtStep(stepNum);
            }
            else
            {
                addBlockAtStep(stepNum, 1);
            }
            return;
        }
    }
    
    // Check control buttons
    if (button == clearButton.get())
    {
        clearPattern();
        if (onPatternCleared)
            onPatternCleared();
    }
    else if (button == randomizeButton.get())
    {
        randomizePattern();
        if (onPatternRandomized)
            onPatternRandomized(2, 6);
    }
}

bool PatternEditorComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Accept step button drags for block editing
    return dragSourceDetails.description.toString().startsWith("Step_");
}

void PatternEditorComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
    // TODO: Implement drag and drop block editing
    DBG("Pattern drag and drop - to be implemented");
}

void PatternEditorComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
}

void PatternEditorComponent::itemDragExit(const SourceDetails& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
}

void PatternEditorComponent::mouseDown(const juce::MouseEvent& event)
{
    // Start drag operation for block editing
    auto stepButton = dynamic_cast<StepButton*>(event.eventComponent);
    if (stepButton)
    {
        isDragging = true;
        dragStartStep = stepButton->getStepNumber();
        dragCurrentStep = dragStartStep;
    }
}

void PatternEditorComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging)
        return;
        
    // Find which step we're over
    for (auto& stepButton : stepButtons)
    {
        if (stepButton->getBounds().contains(event.getPosition()))
        {
            int newStep = stepButton->getStepNumber();
            if (newStep != dragCurrentStep)
            {
                dragCurrentStep = newStep;
                // Visual feedback could be added here
            }
            break;
        }
    }
}

void PatternEditorComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    if (isDragging && dragStartStep != dragCurrentStep)
    {
        // Create block from drag start to current position
        int startStep = juce::jmin(dragStartStep, dragCurrentStep);
        int endStep = juce::jmax(dragStartStep, dragCurrentStep);
        int length = endStep - startStep + 1;
        
        addBlockAtStep(startStep, length);
    }
    
    isDragging = false;
    dragStartStep = -1;
    dragCurrentStep = -1;
}

//==============================================================================
void PatternEditorComponent::setBlocks(const std::vector<BlockData>& newBlocks)
{
    if (blocks != newBlocks)
    {
        blocks = newBlocks;
        updateStepButtons();
        repaint();
    }
}

void PatternEditorComponent::setCurrentStep(int step)
{
    if (currentStep != step)
    {
        currentStep = step;
        updatePlayheadPosition();
        repaint();
    }
}

void PatternEditorComponent::clearPattern()
{
    blocks.clear();
    updateStepButtons();
    repaint();
}

void PatternEditorComponent::randomizePattern(int minBlocks, int maxBlocks)
{
    blocks.clear();
    
    auto& random = juce::Random::getSystemRandom();
    int numBlocks = random.nextInt(juce::Range<int>(minBlocks, maxBlocks + 1));
    
    // Generate random blocks
    for (int i = 0; i < numBlocks; ++i)
    {
        BlockData block;
        block.startStep = random.nextInt(TOTAL_STEPS - 4); // Leave room for block length
        block.lengthSteps = random.nextInt(juce::Range<int>(1, 5)); // 1-4 steps long
        block.chordIndex = random.nextInt(chordCount > 0 ? chordCount : 4); // Random chord
        
        // Make sure block doesn't exceed total steps
        if (block.startStep + block.lengthSteps > TOTAL_STEPS)
        {
            block.lengthSteps = TOTAL_STEPS - block.startStep;
        }
        
        blocks.push_back(block);
    }
    
    updateStepButtons();
    repaint();
}

//==============================================================================
void PatternEditorComponent::setupUI()
{
    // Header label
    headerLabel = std::make_unique<juce::Label>("header", "Pattern Editor");
    headerLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    headerLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    headerLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*headerLabel);
    
    // Clear button
    clearButton = std::make_unique<juce::TextButton>("Clear");
    clearButton->addListener(this);
    clearButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::error);
    clearButton->setAccessible(true);
    clearButton->setTitle("Clear all pattern blocks");
    addAndMakeVisible(*clearButton);
    
    // Randomize button
    randomizeButton = std::make_unique<juce::TextButton>("Randomize");
    randomizeButton->addListener(this);
    randomizeButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::secondary);
    randomizeButton->setAccessible(true);
    randomizeButton->setTitle("Generate random pattern");
    addAndMakeVisible(*randomizeButton);
}

void PatternEditorComponent::setupStepButtons()
{
    stepButtons.clear();
    
    // Create 32 step buttons
    for (int i = 0; i < TOTAL_STEPS; ++i)
    {
        auto button = std::make_unique<StepButton>(i);
        button->addListener(this);
        addAndMakeVisible(*button);
        stepButtons.push_back(std::move(button));
    }
    
    updateStepButtons();
}

void PatternEditorComponent::updateStepButtons()
{
    // Reset all step buttons
    for (auto& stepButton : stepButtons)
    {
        stepButton->setHasBlock(false);
    }
    
    // Set blocks on appropriate steps
    for (const auto& block : blocks)
    {
        auto blockColour = getChordColour(block.chordIndex);
        
        for (int step = block.startStep; step < block.startStep + block.lengthSteps; ++step)
        {
            if (step >= 0 && step < TOTAL_STEPS && step < static_cast<int>(stepButtons.size()))
            {
                stepButtons[static_cast<size_t>(step)]->setHasBlock(true, blockColour);
            }
        }
    }
}

void PatternEditorComponent::updatePlayheadPosition()
{
    // Update current step indicators
    for (size_t i = 0; i < stepButtons.size(); ++i)
    {
        stepButtons[i]->setIsCurrentStep(static_cast<int>(i) == currentStep);
    }
}

//==============================================================================
void PatternEditorComponent::addBlockAtStep(int startStep, int length)
{
    // Remove any existing block at this step first
    removeBlockAtStep(startStep);
    
    // Create new block
    BlockData newBlock;
    newBlock.startStep = startStep;
    newBlock.lengthSteps = juce::jmin(length, TOTAL_STEPS - startStep);
    newBlock.chordIndex = selectedChordIndex;
    
    blocks.push_back(newBlock);
    updateStepButtons();
    
    if (onBlockAdded)
        onBlockAdded(newBlock);
}

void PatternEditorComponent::removeBlockAtStep(int step)
{
    int blockIndex = findBlockIndexAtStep(step);
    if (blockIndex >= 0)
    {
        blocks.erase(blocks.begin() + blockIndex);
        updateStepButtons();
        
        if (onBlockRemoved)
            onBlockRemoved(blockIndex);
    }
}

BlockData* PatternEditorComponent::findBlockAtStep(int step)
{
    for (auto& block : blocks)
    {
        if (step >= block.startStep && step < block.startStep + block.lengthSteps)
        {
            return &block;
        }
    }
    return nullptr;
}

int PatternEditorComponent::findBlockIndexAtStep(int step)
{
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        const auto& block = blocks[i];
        if (step >= block.startStep && step < block.startStep + block.lengthSteps)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

//==============================================================================
juce::Colour PatternEditorComponent::getChordColour(int chordIndex) const
{
    // Cycle through chord colors based on index
    juce::Array<juce::Colour> colours = {
        ModernLookAndFeel::Colors::primary,
        ModernLookAndFeel::Colors::secondary,
        ModernLookAndFeel::Colors::accent,
        ModernLookAndFeel::Colors::info,
        ModernLookAndFeel::Colors::success,
        ModernLookAndFeel::Colors::warning
    };
    
    return colours[chordIndex % colours.size()];
}

void PatternEditorComponent::drawStepNumbers(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    juce::ignoreUnused(area);
    
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(ModernLookAndFeel::Typography::getSmallFont());
    
    // Beat indicators are drawn by step buttons themselves
    // Could add measure markers here if needed
}

void PatternEditorComponent::drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    juce::ignoreUnused(area);
    
    // Playhead is drawn by individual step buttons
    // Could add a global playhead line here if needed
    if (currentStep >= 0 && currentStep < TOTAL_STEPS)
    {
        // Additional playhead visualization could go here
    }
}

} // namespace ChordFoundry