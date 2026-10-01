#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_events/juce_events.h>
#include <functional>
#include "Data/ChordProgression.h"
#include "Data/ProjectFile.h"
#include "UI/ModernLookAndFeel.h"

namespace ChordFoundry {

// Forward declarations
class ChordPanelComponent;
class SettingsPanelComponent;
class StructurePanelComponent;
class PatternEditorComponent;
class ChordSynthesizer;
class MusicTheoryEngine;
class ChordModifierDialog;

//==============================================================================
/*
    MainComponent - Primary application window matching Python MainWindow functionality

    This is the main orchestrating component that:
    - Manages the 3-column layout (chord panel, structure panel, settings panel)
    - Handles the pattern editor below
    - Coordinates between UI components and audio engine
    - Manages chord progression state
    - Starts and stops sequenced playback; the audio engine counts the steps itself, in
      samples, so nothing here keeps time
    - Saves and opens project files, and guards unsaved changes
*/
class MainComponent : public juce::Component,
                     public juce::Timer,
                     public juce::KeyListener,
                     public juce::MenuBarModel,
                     public juce::ApplicationCommandTarget
{
public:
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Timer callback: follows the audio engine's playhead for display (about 30 Hz). It does
    // not decide when steps happen.
    void timerCallback() override;

    // Key listener for keyboard shortcuts
    using juce::Component::keyPressed;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    //==============================================================================
    // Opens the audio output. Separate from the constructor so tools that only render the
    // window (the snapshot mode) never touch audio hardware. If the device cannot be opened
    // the user is told, with a way to open the audio settings.
    void startAudio();

    // Audio settings dialog (device, sample rate, buffer size).
    void showAudioSettings();

    //==============================================================================
    // Project files
    bool hasUnsavedChanges() const;
    juce::File getCurrentProjectFile() const { return currentProjectFile; }
    int getChordCountForCheck() const;
    ChordData getChordForCheck(int index) const;   // for the --project-selfcheck developer mode

    // Asks to save if there are unsaved changes, then calls proceed(true) to carry on or
    // proceed(false) if the user cancelled (or a save failed). Calls proceed(true)
    // immediately when there is nothing to save. Used for quit, New and Open.
    void confirmDiscardChanges(std::function<void(bool proceed)> proceed);

    void newProject();
    void openProject();
    void saveProject(std::function<void(bool saved)> done = nullptr);
    void saveProjectAs(std::function<void(bool saved)> done = nullptr);

    // Loads a project file straight away (no unsaved-changes prompt); reports failure to the
    // user. Returns true on success.
    bool openProjectFile(const juce::File& file);

    //==============================================================================
    // Menu bar and commands
    enum CommandIDs
    {
        cmdNewProject = 0x7001,
        cmdOpenProject,
        cmdSaveProject,
        cmdSaveProjectAs,
        cmdExportMidi,
        cmdAudioSettings
    };

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    //==============================================================================
    // Chord management callbacks (from UI components)
    void onChordSelected(const juce::String& roman);
    void onChordAdded();
    void onChordRemoved(int index);
    void onChordModified(int index, const ChordData& newData);
    void showChordModifiers(int index);
    void onClearAllChords();
    void onRandomizeChords();

    // Playback control callbacks
    void onPlaybackStateChanged(bool shouldPlay);
    void onTempoChanged(float newTempo);
    void onKeyChanged(const juce::String& newKey);
    void onModeChanged(const juce::String& newMode);
    void onClickTrackChanged(bool enabled);
    void onLoopChanged(bool enabled);
    void onVolumeChanged(float newVolume);

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

    // Text description of where the panels sit and whether anything leaves the visible
    // area. Used by the snapshot mode to check the layout without a person looking at it.
    juce::String describeLayout() const;

    // The smallest content size the panels are laid out for. Below it the content scrolls.
    static constexpr int MIN_CONTENT_WIDTH = 1120;
    static constexpr int MIN_CONTENT_HEIGHT = 590;

private:
    class ContentArea;

    //==============================================================================
    // UI Components
    std::unique_ptr<ChordPanelComponent> chordPanel;
    std::unique_ptr<StructurePanelComponent> structurePanel;
    std::unique_ptr<SettingsPanelComponent> settingsPanel;
    std::unique_ptr<PatternEditorComponent> patternEditor;

    // The panels live in a scrolling area so a window smaller than the layout needs scrolls
    // instead of clipping.
    std::unique_ptr<ContentArea> contentArea;
    juce::Viewport contentViewport;

    // Header toolbar
    juce::TextButton newButton { "New" };
    juce::TextButton openButton { "Open" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton saveAsButton { "Save As" };
    juce::TextButton audioButton { "Audio Settings" };
    std::unique_ptr<juce::MenuBarComponent> menuBar; // in-window menu bar where there is no system menu bar
    juce::TooltipWindow tooltipWindow { this, 600 };

    // Modern styling
    std::unique_ptr<ModernLookAndFeel> modernLookAndFeel;

    // Application state
    std::unique_ptr<ChordProgression> chordProgression;
    std::unique_ptr<ChordSynthesizer> synthesizer;

    // File chooser for export / open / save
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Component::SafePointer<juce::DialogWindow> audioSettingsWindow;
    juce::Component::SafePointer<juce::DialogWindow> modifierWindow;
    juce::ApplicationCommandManager commandManager;

    // Current settings
    juce::String selectedRoman;
    juce::String currentKey = "C";
    juce::String currentMode = "Major";
    float currentTempo = 120.0f;
    float masterVolume = 0.7f;
    bool isPlaying = false;
    bool loopEnabled = false;
    bool clickTrackEnabled = false;

    // Project file state
    juce::File currentProjectFile;
    juce::String savedSnapshot;     // text of the project as last saved or opened
    bool audioProblemShowing = false;

    // Playback display state (the audio engine owns the real position)
    int currentStep = -1;
    int finishedCountAtStart = 0;

    // Layout constants for modern responsive design
    static constexpr int PANEL_SPACING = ModernLookAndFeel::Metrics::spacingMD;
    static constexpr int WINDOW_PADDING = ModernLookAndFeel::Metrics::spacingMD;
    static constexpr int HEADER_HEIGHT = ModernLookAndFeel::Metrics::headerHeight;
    static constexpr int STATUS_BAR_HEIGHT = 30;
    static constexpr int MENU_BAR_HEIGHT = 24;

    //==============================================================================
    // Setup methods
    void setupCallbacks();
    void setupCommands();
    void setupToolbar();

    // Layout methods
    void layoutPanels(juce::Rectangle<int> bounds);
    void updateContentSize();
    juce::Rectangle<int> getHeaderBounds() const;
    int getMenuBarHeight() const;

    // Playback methods
    void startPlayback();
    void stopPlayback();
    void updatePlayheadDisplay();

    // Audio methods
    void stopAllNotes();
    void refreshSequencerPattern();
    void showAudioProblem();

    // State management
    void updateChordPanelState();
    void updateStructurePanelState();
    void updatePatternEditorState();
    void modelChanged();

    // File operations
    ProjectSettings getCurrentSettings() const;
    juce::String getCurrentSnapshot() const;
    void applyProject(const ProjectSettings& settings);
    bool writeProjectFile(const juce::File& file);
    void updateWindowTitle();
    juce::String getProjectName() const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace ChordFoundry
