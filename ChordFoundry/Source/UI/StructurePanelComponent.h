#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Data/ChordProgression.h"
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * StructurePanelComponent - Modern chord progression display and management interface
 * 
 * Features:
 * - Visual chord progression display with Roman numerals
 * - Drag and drop chord reordering
 * - Individual chord editing and deletion
 * - Chord analysis and suggestions
 * - Export options for progression
 * - Playback position indicator
 */
class StructurePanelComponent : public juce::Component,
                               public juce::Button::Listener,
                               public juce::DragAndDropTarget
{
public:
    StructurePanelComponent();
    ~StructurePanelComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button listener
    void buttonClicked(juce::Button* button) override;
    
    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    
    // Public API
    void setChordProgression(const std::vector<ChordData>& chords);
    void setCurrentChordIndex(int index);
    void clearProgression();
    void addChord(const ChordData& chord);
    void removeChord(int index);
    void updateChord(int index, const ChordData& newChord);
    
    // Callbacks
    std::function<void(int)> onChordSelected;
    std::function<void(int)> onChordRemoved;
    std::function<void(int, const ChordData&)> onChordModified;
    std::function<void()> onClearAll;
    std::function<void()> onRandomizeProgression;
    std::function<void()> onExportProgression;
    
private:
    //==============================================================================
    // Chord display button class
    class ChordButton : public juce::Button
    {
    public:
        ChordButton(const ChordData& chord, int index);
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, 
                        bool shouldDrawButtonAsDown) override;
        
        void setChordData(const ChordData& newChord) { chordData = newChord; repaint(); }
        const ChordData& getChordData() const { return chordData; }
        int getChordIndex() const { return chordIndex; }
        void setChordIndex(int index) { chordIndex = index; }
        void setIsCurrentChord(bool isCurrent) { isCurrentChord = isCurrent; repaint(); }
        
    private:
        ChordData chordData;
        int chordIndex;
        bool isCurrentChord = false;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordButton)
    };
    
    //==============================================================================
    // Data
    std::vector<ChordData> chords;
    std::vector<std::unique_ptr<ChordButton>> chordButtons;
    int currentChordIndex = -1;
    
    // UI elements
    std::unique_ptr<juce::Label> headerLabel;
    std::unique_ptr<juce::TextButton> clearButton;
    std::unique_ptr<juce::TextButton> randomizeButton;
    std::unique_ptr<juce::TextButton> exportButton;
    std::unique_ptr<juce::Viewport> chordViewport;
    std::unique_ptr<juce::Component> chordContainer;
    
    // Layout constants
    static constexpr int CHORD_BUTTON_WIDTH = 80;
    static constexpr int CHORD_BUTTON_HEIGHT = 60;
    static constexpr int BUTTONS_PER_ROW = 4;
    
    //==============================================================================
    void setupUI();
    void updateChordButtons();
    void updateChordButtonLayout();
    void updateProgressionAnalysis();
    
    // Chord analysis helpers
    juce::String analyzeProgression() const;
    juce::String getChordFunctionAnalysis(const ChordData& chord, int index) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StructurePanelComponent)
};

} // namespace ChordFoundry
