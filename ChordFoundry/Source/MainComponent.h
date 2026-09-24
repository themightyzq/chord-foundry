#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_events/juce_events.h>
#include "Data/ChordProgression.h"
#include "UI/ModernLookAndFeel.h"

namespace ChordFoundry {

// Forward declarations
class ChordPanelComponent;
class SettingsPanelComponent;
class StructurePanelComponent;  
class PatternEditorComponent;
class ChordSynthesizer;
class MusicTheoryEngine;

//==============================================================================
/*
    MainComponent - Primary application window matching Python MainWindow functionality
    
    This is the main orchestrating component that:
    - Manages the 3-column layout (chord panel, structure panel, settings panel)
    - Handles the pattern editor below
    - Coordinates between UI components and audio engine
    - Manages chord progression state
    - Provides playback control and timing
*/
class MainComponent : public juce::Component,
                     public juce::Timer,
                     public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Timer callback for playback updates
    void timerCallback() override;
    
    // Key listener for keyboard shortcuts
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    //==============================================================================
    // Chord management callbacks (from UI components)
    void onChordSelected(const juce::String& roman);
    void onChordAdded();
    void onChordRemoved(int index);
    void onChordModified(int index, const ChordData& newData);
    void onClearAllChords();
    void onRandomizeChords();
    
    // Playback control callbacks
    void onPlaybackStateChanged(bool shouldPlay);
    void onTempoChanged(float newTempo);
    void onKeyChanged(const juce::String& newKey);
    void onModeChanged(const juce::String& newMode);
    void onClickTrackChanged(bool enabled);
    void onLoopChanged(bool enabled);
    
    // Pattern editor callbacks
    void onBlockAdded(const BlockData& block);
    void onBlockRemoved(int blockIndex);
    void onBlockModified(int blockIndex, const BlockData& newBlock);
    void onPatternRandomized(int minBlocks, int maxBlocks);
    void onPatternCleared();
    
    // Export functionality
    void onExportMidi();
    
    // Audio preview (individual chord playback)
    void playChordPreview(int chordIndex);
    void stopChordPreview();

private:
    //==============================================================================
    // UI Components
    std::unique_ptr<ChordPanelComponent> chordPanel;
    std::unique_ptr<StructurePanelComponent> structurePanel;
    std::unique_ptr<SettingsPanelComponent> settingsPanel;
    std::unique_ptr<PatternEditorComponent> patternEditor;
    
    // Modern styling
    std::unique_ptr<ModernLookAndFeel> modernLookAndFeel;
    
    // Application state
    std::unique_ptr<ChordProgression> chordProgression;
    std::unique_ptr<ChordSynthesizer> synthesizer;
    
    // File chooser for export
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // Current settings
    juce::String selectedRoman;
    juce::String currentKey = "C";
    juce::String currentMode = "Major";
    float currentTempo = 120.0f;
    bool isPlaying = false;
    bool loopEnabled = false;
    bool clickTrackEnabled = false;
    
    // Playback state
    int currentStep = 0;
    double stepDurationMs = 125.0; // Default for 120 BPM (60/120/4 * 1000)
    juce::int64 lastStepTime = 0;
    
    // Layout constants for modern responsive design
    static constexpr int CHORD_PANEL_WIDTH = 400;
    static constexpr int SETTINGS_PANEL_WIDTH = 320;
    static constexpr int PANEL_HEIGHT = 640;
    static constexpr int PATTERN_EDITOR_HEIGHT = 320;
    static constexpr int PANEL_SPACING = ModernLookAndFeel::Metrics::spacingLG;
    static constexpr int WINDOW_PADDING = ModernLookAndFeel::Metrics::spacingXL;
    static constexpr int HEADER_HEIGHT = ModernLookAndFeel::Metrics::headerHeight;
    static constexpr int MIN_WINDOW_WIDTH = 1000;
    static constexpr int MIN_WINDOW_HEIGHT = 800;
    
    //==============================================================================
    // Setup methods
    void setupLayout();
    void setupCallbacks();
    void setupKeyboardShortcuts();
    void setupInitialState();
    
    // Layout methods
    void layoutHorizontally(juce::Rectangle<int> bounds);
    void layoutVertically(juce::Rectangle<int> bounds);
    
    // Playback methods
    void startPlayback();
    void stopPlayback();
    void processCurrentStep();
    void updatePlayheadDisplay();
    void calculateStepDuration();
    
    // Audio methods
    void playNotesForStep(int step);
    void stopAllNotes();
    void playClickTrack(int step);
    
    // State management
    void updateChordPanelState();
    void updateStructurePanelState();
    void updatePatternEditorState();
    void validateState();
    
    // File operations
    bool saveProject(const juce::File& file);
    bool loadProject(const juce::File& file);
    void showSaveDialog();
    void showLoadDialog();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace ChordFoundry