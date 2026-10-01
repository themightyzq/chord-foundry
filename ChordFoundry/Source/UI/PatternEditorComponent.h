#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Data/ChordProgression.h"
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * PatternEditorComponent - Modern 32-step pattern sequencer interface
 * 
 * Features:
 * - 32-step grid for pattern creation
 * - Visual block editing with the mouse: drag across empty steps to draw a block, drag a
 *   block to move it, drag the right edge of its last step to resize it, click a block to
 *   remove it, Shift-click its first step to toggle "strike again"

 * - Real-time playback position indicator
 * - Block length and position adjustment
 * - Chord assignment per block
 * - Pattern randomization and templates
 */
class PatternEditorComponent : public juce::Component,
                              public juce::TooltipClient,
                              public juce::Button::Listener,
                              public juce::DragAndDropTarget
{
public:
    PatternEditorComponent();
    ~PatternEditorComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    // Tooltip for the step under the mouse (the step buttons themselves do not take the mouse).
    juce::String getTooltip() override;

    // Bounds of a step button in this component's coordinates (empty if out of range).
    juce::Rectangle<int> getStepBounds(int step) const;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    
    // Mouse handling for block editing
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    // Public API
    void setBlocks(const std::vector<BlockData>& blocks);
    const std::vector<BlockData>& getAllBlocks() const { return blocks; }
    void setCurrentStep(int step);

    // Blocks drawn on the grid use this chord. The chord count keeps the index in range.
    void setSelectedChordIndex(int index);
    void setChordCount(int count);
    int getSelectedChordIndex() const { return selectedChordIndex; }
    void clearPattern();
    void randomizePattern(int minBlocks = 2, int maxBlocks = 6);
    
    // Callbacks
    std::function<void(const BlockData&)> onBlockAdded;
    std::function<void(int)> onBlockRemoved;
    std::function<void(int, const BlockData&)> onBlockModified;
    std::function<void()> onPatternCleared;
    std::function<void(int, int)> onPatternRandomized;
    
private:
    //==============================================================================
    // Step button class for the 32-step grid
    class StepButton : public juce::Button
    {
    public:
        StepButton(int stepNumber);
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, 
                        bool shouldDrawButtonAsDown) override;
        
        void setIsCurrentStep(bool isCurrent) { isCurrentStep = isCurrent; repaint(); }
        void setIsStrike(bool strike) { if (isStrike != strike) { isStrike = strike; repaint(); } }
        void setHasBlock(bool hasBlock, const juce::Colour& blockColour = juce::Colours::blue) 
        { 
            this->hasBlock = hasBlock; 
            this->blockColour = blockColour;
            repaint(); 
        }
        
        int getStepNumber() const { return stepNumber; }
        
    private:
        int stepNumber;
        bool isCurrentStep = false;
        bool isStrike = false;   // first step of a block flagged to re-strike its chord
        bool hasBlock = false;
        juce::Colour blockColour = ModernLookAndFeel::Colors::primary;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepButton)
    };
    
    //==============================================================================
    // Data
    std::vector<BlockData> blocks;
    int currentStep = -1;
    int chordCount = 0;
    
    // UI elements
    std::unique_ptr<juce::Label> headerLabel;
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::TextButton> randomizeButton;
    std::vector<std::unique_ptr<StepButton>> stepButtons;
    
    // Editing state
    enum class DragMode { none, draw, move, resize };
    DragMode dragMode = DragMode::none;
    int dragBlockIndex = -1;       // block being moved or resized
    int dragGrabOffset = 0;        // steps from the block's start to where it was grabbed
    int dragStartStep = -1;        // step the mouse went down on
    int dragCurrentStep = -1;      // step the mouse is over now
    bool dragTravelled = false;    // the mouse reached a step other than the one it went down on
    BlockData dragOriginal;        // the block as it was when the drag began
    int hoveredStep = -1;
    int selectedChordIndex = 0;
    
    // Layout constants
    static constexpr int STEPS_PER_ROW = 16;
    static constexpr int TOTAL_STEPS = 32;
    static constexpr int STEP_BUTTON_SIZE = 24;
    static constexpr int GRID_SPACING = 2;
    
    //==============================================================================
    void setupUI();
    void setupStepButtons();
    void updateStepButtons();
    int getHeaderHeight() const;
    int getButtonAreaHeight() const;
    void updatePlayheadPosition();

    // Mouse helpers
    int getStepAt(juce::Point<int> position, bool clampToGrid) const;
    bool isInResizeZone(juce::Point<int> position, int step) const;
    void handleStepClick(int step, bool shiftDown);
    void updateHover(int step);
    
    // Block management
    void addBlockAtStep(int startStep, int length = 1, bool replaceExisting = true);
    void removeBlockAtStep(int step);
    BlockData* findBlockAtStep(int step);
    int findBlockIndexAtStep(int step) const;
    
    // Visual helpers
    juce::Colour getChordColour(int chordIndex) const;
    void drawStepNumbers(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& area);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternEditorComponent)
};

} // namespace ChordFoundry