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
    
    // Mark the steps that re-strike their chord with a small triangle in the top-left corner
    if (isStrike)
    {
        juce::Path marker;
        marker.addTriangle(bounds.getX() + 1.0f, bounds.getY() + 1.0f,
                           bounds.getX() + 11.0f, bounds.getY() + 1.0f,
                           bounds.getX() + 1.0f, bounds.getY() + 11.0f);
        g.setColour(ModernLookAndFeel::Colors::textOnPrimary);
        g.fillPath(marker);
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
    gridArea.removeFromTop(getHeaderHeight()); // Header
    gridArea.removeFromBottom(getButtonAreaHeight()); // Buttons
    
    // Draw step numbers and beat indicators
    drawStepNumbers(g, gridArea);
    
    // Draw playhead if playing
    if (currentStep >= 0)
    {
        drawPlayhead(g, gridArea);
    }
}

void PatternEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    
    // Header area
    auto headerBounds = bounds.removeFromTop(getHeaderHeight());
    if (headerLabel)
        headerLabel->setBounds(headerBounds);
    
    // Button area at bottom
    auto buttonArea = bounds.removeFromBottom(getButtonAreaHeight());
    buttonArea.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
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

int PatternEditorComponent::getHeaderHeight() const
{
    return getHeight() < 300 ? 36 : 60;
}

int PatternEditorComponent::getButtonAreaHeight() const
{
    return ModernLookAndFeel::Metrics::buttonHeight + ModernLookAndFeel::Metrics::spacingSM;
}

//==============================================================================
void PatternEditorComponent::buttonClicked(juce::Button* button)
{
    // Step buttons only see keyboard activation (the mouse is handled by this component).
    for (size_t i = 0; i < stepButtons.size(); ++i)
    {
        if (stepButtons[i].get() == button)
        {
            handleStepClick(static_cast<int>(i), juce::ModifierKeys::currentModifiers.isShiftDown());
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

void PatternEditorComponent::handleStepClick(int stepNum, bool shiftDown)
{
    const int index = findBlockIndexAtStep(stepNum);

    // Shift-click on the first step of a block: toggle "strike again" for that block
    if (shiftDown)
    {
        if (index >= 0 && blocks[static_cast<size_t>(index)].startStep == stepNum)
        {
            auto changed = blocks[static_cast<size_t>(index)];
            changed.newStrike = !changed.newStrike;
            blocks[static_cast<size_t>(index)] = changed;
            updateStepButtons();

            if (onBlockModified)
                onBlockModified(index, changed);
        }
        return;
    }

    // A plain click toggles a one-step block
    if (index >= 0)
        removeBlockAtStep(stepNum);
    else
        addBlockAtStep(stepNum, 1);
}

int PatternEditorComponent::getStepAt(juce::Point<int> position, bool clampToGrid) const
{
    int best = -1;
    float bestDistance = 0.0f;

    for (size_t i = 0; i < stepButtons.size(); ++i)
    {
        const auto bounds = stepButtons[i]->getBounds();
        if (bounds.contains(position))
            return static_cast<int>(i);

        if (clampToGrid)
        {
            const float distance = position.toFloat().getDistanceFrom(bounds.getConstrainedPoint(position).toFloat());
            if (best < 0 || distance < bestDistance)
            {
                best = static_cast<int>(i);
                bestDistance = distance;
            }
        }
    }

    return best;
}

bool PatternEditorComponent::isInResizeZone(juce::Point<int> position, int step) const
{
    if (step < 0 || step >= static_cast<int>(stepButtons.size()))
        return false;

    // The right-hand part of a block's last step is its resize handle.
    const auto bounds = stepButtons[static_cast<size_t>(step)]->getBounds();
    const int index = findBlockIndexAtStep(step);
    if (index < 0)
        return false;

    const auto& block = blocks[static_cast<size_t>(index)];
    return step == block.startStep + block.lengthSteps - 1
        && position.x >= bounds.getRight() - juce::jmax(10, bounds.getWidth() / 3);
}

juce::Rectangle<int> PatternEditorComponent::getStepBounds(int step) const
{
    if (step < 0 || step >= static_cast<int>(stepButtons.size()))
        return {};

    return stepButtons[static_cast<size_t>(step)]->getBounds();
}

void PatternEditorComponent::mouseDown(const juce::MouseEvent& event)
{
    dragMode = DragMode::none;
    const int step = getStepAt(event.getPosition(), false);
    if (step < 0)
        return;

    const int index = findBlockIndexAtStep(step);

    // Right-click removes the block under the mouse
    if (event.mods.isPopupMenu())
    {
        if (index >= 0)
            removeBlockAtStep(step);
        return;
    }

    dragStartStep = step;
    dragCurrentStep = step;
    dragTravelled = false;

    if (index >= 0)
    {
        dragBlockIndex = index;
        dragOriginal = blocks[static_cast<size_t>(index)];
        dragGrabOffset = step - dragOriginal.startStep;
        dragMode = isInResizeZone(event.getPosition(), step) ? DragMode::resize : DragMode::move;
    }
    else
    {
        dragMode = DragMode::draw;
    }
}

void PatternEditorComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (dragMode == DragMode::none)
        return;

    const int step = getStepAt(event.getPosition(), true);
    if (step < 0 || step == dragCurrentStep)
        return;

    dragCurrentStep = step;
    dragTravelled = true;

    if (dragMode == DragMode::move)
    {
        auto candidate = dragOriginal;
        candidate.startStep = ChordProgression::clampMoveStart(dragOriginal, step - dragGrabOffset);

        // Stay at the last position that is free (it will not jump over another block of the same chord).
        if (ChordProgression::isPlacementFree(blocks, dragBlockIndex, candidate))
        {
            blocks[static_cast<size_t>(dragBlockIndex)] = candidate;
            updateStepButtons();
        }
    }
    else if (dragMode == DragMode::resize)
    {
        auto candidate = blocks[static_cast<size_t>(dragBlockIndex)];
        candidate.lengthSteps = juce::jlimit(1, ChordProgression::maxResizeLength(blocks, dragBlockIndex),
                                             step - candidate.startStep + 1);
        blocks[static_cast<size_t>(dragBlockIndex)] = candidate;
        updateStepButtons();
    }

    repaint();
}

void PatternEditorComponent::mouseUp(const juce::MouseEvent& event)
{
    const auto mode = dragMode;
    const int index = dragBlockIndex;
    const int startStep = dragStartStep;
    const int endStep = dragCurrentStep;
    const bool travelled = dragTravelled;

    dragMode = DragMode::none;
    dragBlockIndex = -1;
    dragStartStep = -1;
    dragCurrentStep = -1;
    repaint();

    if (mode == DragMode::none || startStep < 0)
        return;

    if (mode == DragMode::draw)
    {
        if (startStep == endStep)
        {
            handleStepClick(startStep, event.mods.isShiftDown());
            return;
        }

        // Draw a block across the dragged steps, stopping before any block of the same chord.
        const int first = juce::jmin(startStep, endStep);
        const int wanted = std::abs(endStep - startStep) + 1;
        const int length = ChordProgression::freeRunLength(blocks, selectedChordIndex, first, wanted);

        if (length > 0)
            addBlockAtStep(first, length, false);
        return;
    }

    if (index < 0 || index >= static_cast<int>(blocks.size()))
        return;

    const auto& current = blocks[static_cast<size_t>(index)];
    const bool changed = current.startStep != dragOriginal.startStep || current.lengthSteps != dragOriginal.lengthSteps;

    if (changed)
    {
        // Hand the finished block to the owner; it validates it and the editor then shows what it kept.
        if (onBlockModified)
            onBlockModified(index, current);
    }
    else if (! travelled)
    {
        handleStepClick(startStep, event.mods.isShiftDown());
    }
    // else: the drag went nowhere that was allowed; the block stays as it was
}

void PatternEditorComponent::updateHover(int step)
{
    if (step == hoveredStep)
        return;

    if (hoveredStep >= 0 && hoveredStep < static_cast<int>(stepButtons.size()))
        stepButtons[static_cast<size_t>(hoveredStep)]->setState(juce::Button::buttonNormal);

    hoveredStep = step;

    if (hoveredStep >= 0 && hoveredStep < static_cast<int>(stepButtons.size()))
        stepButtons[static_cast<size_t>(hoveredStep)]->setState(juce::Button::buttonOver);
}

void PatternEditorComponent::mouseMove(const juce::MouseEvent& event)
{
    const int step = getStepAt(event.getPosition(), false);
    updateHover(step);

    if (step >= 0 && isInResizeZone(event.getPosition(), step))
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (step >= 0 && findBlockIndexAtStep(step) >= 0)
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PatternEditorComponent::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    updateHover(-1);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

juce::String PatternEditorComponent::getTooltip()
{
    const int step = getStepAt(getMouseXYRelative(), false);
    if (step < 0)
        return {};

    const int index = findBlockIndexAtStep(step);
    juce::String text = "Step " + juce::String(step + 1) + ". ";

    if (index < 0)
        return text + "Click to add a one-step block of the selected chord, or drag across steps to draw a longer one.";

    return text + "Drag the block to move it, drag the right edge of its last step to resize it, "
                  "click to remove it. Shift-click its first step to strike the chord again "
                  "instead of sustaining the one before. Right-click also removes it.";
}

void PatternEditorComponent::paintOverChildren(juce::Graphics& g)
{
    if (dragMode != DragMode::draw || dragStartStep < 0 || dragCurrentStep < 0 || dragStartStep == dragCurrentStep)
        return;

    const int first = juce::jmin(dragStartStep, dragCurrentStep);
    const int wanted = std::abs(dragCurrentStep - dragStartStep) + 1;
    const int length = ChordProgression::freeRunLength(blocks, selectedChordIndex, first, wanted);

    g.setColour(ModernLookAndFeel::Colors::borderFocus);
    for (int step = first; step < first + length; ++step)
        g.drawRect(getStepBounds(step), 2);
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

void PatternEditorComponent::setSelectedChordIndex(int index)
{
    selectedChordIndex = juce::jmax(0, chordCount > 0 ? juce::jmin(index, chordCount - 1) : index);
}

void PatternEditorComponent::setChordCount(int count)
{
    chordCount = juce::jmax(0, count);

    if (chordCount > 0 && selectedChordIndex >= chordCount)
        selectedChordIndex = chordCount - 1;
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
    clearButton->setTooltip("Remove every block from the pattern");
    addAndMakeVisible(*clearButton);
    
    // Randomize button
    randomizeButton = std::make_unique<juce::TextButton>("Randomize");
    randomizeButton->addListener(this);
    randomizeButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::secondary);
    randomizeButton->setAccessible(true);
    randomizeButton->setTitle("Generate random pattern");
    randomizeButton->setTooltip("Fill the pattern with random blocks of the chords in the progression");
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
        button->setInterceptsMouseClicks(false, false);   // this component handles the mouse for the whole grid
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
        stepButton->setIsStrike(false);
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

        if (block.newStrike && block.startStep >= 0 && block.startStep < static_cast<int>(stepButtons.size()))
            stepButtons[static_cast<size_t>(block.startStep)]->setIsStrike(true);
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
void PatternEditorComponent::addBlockAtStep(int startStep, int length, bool replaceExisting)
{
    // A click replaces whatever block is at this step; a drawn block was already checked to be free
    if (replaceExisting)
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

int PatternEditorComponent::findBlockIndexAtStep(int step) const
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
    juce::ignoreUnused(g, area);
    
    // Playhead is drawn by individual step buttons
    // Could add a global playhead line here if needed
    if (currentStep >= 0 && currentStep < TOTAL_STEPS)
    {
        // Additional playhead visualization could go here
    }
}

} // namespace ChordFoundry