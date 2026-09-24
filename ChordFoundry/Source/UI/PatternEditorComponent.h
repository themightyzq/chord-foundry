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
 * - Visual block editing with drag and drop
 * - Real-time playback position indicator
 * - Block length and position adjustment
 * - Chord assignment per block
 * - Pattern randomization and templates
 */
class PatternEditorComponent : public juce::Component,
                              public juce::Button::Listener,
                              public juce::DragAndDropTarget
{
public:
    PatternEditorComponent();
    ~PatternEditorComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
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
    
    // Public API
    void setBlocks(const std::vector<BlockData>& blocks);
    const std::vector<BlockData>& getAllBlocks() const { return blocks; }
    void setCurrentStep(int step);
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
    bool isDragging = false;
    int dragStartStep = -1;
    int dragCurrentStep = -1;
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
    void updatePlayheadPosition();
    
    // Block management
    void addBlockAtStep(int startStep, int length = 1);
    void removeBlockAtStep(int step);
    BlockData* findBlockAtStep(int step);
    int findBlockIndexAtStep(int step);
    
    // Visual helpers
    juce::Colour getChordColour(int chordIndex) const;
    void drawStepNumbers(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& area);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternEditorComponent)
};

} // namespace ChordFoundry