#include "MainComponent.h"
#include "UI/ChordPanelComponent.h"
#include "UI/SettingsPanelComponent.h"
#include "UI/StructurePanelComponent.h"
#include "UI/PatternEditorComponent.h"
#include "Audio/ChordSynthesizer.h"
#include "MusicTheory/MusicTheoryEngine.h"
#include "Export/MidiExporter.h"

namespace ChordFoundry {

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize modern look and feel
    modernLookAndFeel = std::make_unique<ModernLookAndFeel>();
    setLookAndFeel(modernLookAndFeel.get());
    
    // Initialize core data structures
    chordProgression = std::make_unique<ChordProgression>();
    
    // Initialize synthesizer and open the default audio output device
    synthesizer = std::make_unique<ChordSynthesizer>();
    synthesizer->start();
    
    // Initialize UI components
    chordPanel = std::make_unique<ChordPanelComponent>();
    settingsPanel = std::make_unique<SettingsPanelComponent>();
    structurePanel = std::make_unique<StructurePanelComponent>();
    patternEditor = std::make_unique<PatternEditorComponent>();
    
    // Set up modern styling for components
    chordPanel->setLookAndFeel(modernLookAndFeel.get());
    settingsPanel->setLookAndFeel(modernLookAndFeel.get());
    structurePanel->setLookAndFeel(modernLookAndFeel.get());
    patternEditor->setLookAndFeel(modernLookAndFeel.get());
    
    // Add components to hierarchy
    addAndMakeVisible(*chordPanel);
    addAndMakeVisible(*settingsPanel);
    addAndMakeVisible(*structurePanel);
    addAndMakeVisible(*patternEditor);
    
    setupLayout();
    setupCallbacks();
    setupKeyboardShortcuts();
    setupInitialState();
    
    // Set up keyboard listener and accessibility
    addKeyListener(this);
    setWantsKeyboardFocus(true);
    setAccessible(true);
    setTitle("Chord Foundry - Professional chord progression and sequencing tool");
    setDescription("Main application window with chord selection, structure editing, and pattern sequencing");
    
    // Set initial size with modern proportions
    auto totalWidth = CHORD_PANEL_WIDTH + SETTINGS_PANEL_WIDTH + 500 + (PANEL_SPACING * 3) + (WINDOW_PADDING * 2);
    auto totalHeight = HEADER_HEIGHT + PANEL_HEIGHT + PATTERN_EDITOR_HEIGHT + PANEL_SPACING + (WINDOW_PADDING * 2);
    setSize(totalWidth, totalHeight);
    
    // Note: Constrainer would be set by parent window if needed
    // This component doesn't directly manage window constraints
}

MainComponent::~MainComponent()
{
    // Clean up look and feel
    setLookAndFeel(nullptr);
    
    stopTimer();
    if (isPlaying) {
        stopPlayback();
    }

    if (synthesizer) {
        synthesizer->stop();
    }
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Create sophisticated dark gradient background typical of professional audio apps
    auto bounds = getLocalBounds().toFloat();
    
    // Multi-stop gradient for depth and visual interest
    juce::ColourGradient backgroundGradient(
        ModernLookAndFeel::Colors::background.brighter(0.02f), bounds.getTopLeft(),
        ModernLookAndFeel::Colors::background.darker(0.03f), bounds.getBottomLeft(), false);
    
    // Add intermediate color stops for more sophisticated gradient
    backgroundGradient.addColour(0.3, ModernLookAndFeel::Colors::background);
    backgroundGradient.addColour(0.7, ModernLookAndFeel::Colors::background.darker(0.01f));
    
    g.setGradientFill(backgroundGradient);
    g.fillRect(bounds);
    
    // Draw header area with professional gradient
    auto headerBounds = getLocalBounds().removeFromTop(HEADER_HEIGHT);
    
    juce::ColourGradient headerGradient(
        ModernLookAndFeel::Colors::surfaceElevated.brighter(0.05f), headerBounds.getTopLeft().toFloat(),
        ModernLookAndFeel::Colors::surfaceElevated.darker(0.1f), headerBounds.getBottomLeft().toFloat(), false);
    
    g.setGradientFill(headerGradient);
    g.fillRect(headerBounds);
    
    // Add subtle inner glow to header
    g.setColour(ModernLookAndFeel::Colors::primary.withAlpha(0.05f));
    g.fillRect(headerBounds.removeFromTop(2));
    
    // Draw header shadow with more depth
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(headerBounds.removeFromBottom(2));
    
    // Draw title with enhanced typography and subtle glow
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(ModernLookAndFeel::Typography::getHeaderFont());
    
    auto titleBounds = headerBounds.reduced(WINDOW_PADDING, 0);
    
    // Add subtle text glow for premium feel
    g.setColour(ModernLookAndFeel::Colors::primary.withAlpha(0.1f));
    g.drawText("Chord Foundry", titleBounds.translated(1, 1), juce::Justification::centred);
    
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.drawText("Chord Foundry", titleBounds, juce::Justification::centred);
    
    // Draw subtitle with improved contrast
    g.setColour(ModernLookAndFeel::Colors::textSecondary.brighter(0.3f));
    g.setFont(ModernLookAndFeel::Typography::getCaptionFont());
    auto subtitleBounds = titleBounds.removeFromBottom(20);
    g.drawText("Professional chord progression and sequencing tool", 
               subtitleBounds, juce::Justification::centred);
    
    // Enhanced status bar with gradient background
    auto statusBounds = getLocalBounds().removeFromBottom(30);
    
    // Status bar gradient for professional appearance
    juce::ColourGradient statusGradient(
        ModernLookAndFeel::Colors::surfaceElevated.brighter(0.03f), statusBounds.getTopLeft().toFloat(),
        ModernLookAndFeel::Colors::surfaceElevated.darker(0.02f), statusBounds.getBottomLeft().toFloat(), false);
    
    g.setGradientFill(statusGradient);
    g.fillRect(statusBounds);
    
    // Add subtle top border
    g.setColour(ModernLookAndFeel::Colors::border.brighter(0.2f));
    g.fillRect(statusBounds.removeFromTop(1));
    
    // Draw status text with improved hierarchy
    auto textBounds = statusBounds.reduced(WINDOW_PADDING, 0);
    g.setFont(ModernLookAndFeel::Typography::getSmallFont());
    
    juce::String stateText = "Key: " + currentKey + " | Mode: " + currentMode + 
                           " | Tempo: " + juce::String(currentTempo, 1) + " BPM";
    
    if (isPlaying) {
        stateText += " | ▶ Playing (Step " + juce::String(currentStep + 1) + "/32)";
        g.setColour(ModernLookAndFeel::Colors::success.brighter(0.2f));
    } else {
        g.setColour(ModernLookAndFeel::Colors::textSecondary.brighter(0.1f));
    }
    
    g.drawText(stateText, textBounds, juce::Justification::centredLeft);
    
    // Enhanced playback indicator with glow effect
    if (isPlaying) {
        auto indicatorBounds = statusBounds.removeFromRight(60).reduced(10, 8);
        auto dotBounds = indicatorBounds.removeFromLeft(12).toFloat();
        
        // Add glow around the live indicator
        g.setColour(ModernLookAndFeel::Colors::success.withAlpha(0.3f));
        g.fillEllipse(dotBounds.expanded(2.0f));
        
        g.setColour(ModernLookAndFeel::Colors::success);
        g.fillEllipse(dotBounds);
        
        g.setColour(ModernLookAndFeel::Colors::success.brighter(0.2f));
        g.setFont(ModernLookAndFeel::Typography::getSmallFont().boldened());
        g.drawText("LIVE", indicatorBounds, juce::Justification::centredLeft);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    auto totalWidth = bounds.getWidth();
    auto totalHeight = bounds.getHeight();
    
    // Ensure minimum usable dimensions
    if (totalWidth < MIN_WINDOW_WIDTH || totalHeight < MIN_WINDOW_HEIGHT)
    {
        // Hide non-essential elements if window is too small
        DBG("Window too small for optimal layout: " + juce::String(totalWidth) + "x" + juce::String(totalHeight));
    }
    
    // Reserve space for header and status bar
    bounds.removeFromTop(HEADER_HEIGHT);
    auto statusBarHeight = 30;
    bounds.removeFromBottom(statusBarHeight);
    
    // Apply responsive window padding based on screen size
    auto responsivePadding = juce::jmax(8, juce::jmin(WINDOW_PADDING, totalWidth / 40));
    bounds = bounds.reduced(responsivePadding);
    
    // Determine layout mode based on available space
    bool useVerticalLayout = totalWidth < 900 || totalHeight < 700;
    
    if (useVerticalLayout)
    {
        layoutVertically(bounds);
    }
    else
    {
        layoutHorizontally(bounds);
    }
}

void MainComponent::layoutHorizontally(juce::Rectangle<int> bounds)
{
    // Traditional 3-column layout for larger screens
    auto panelArea = bounds.removeFromTop(PANEL_HEIGHT);
    auto availableWidth = panelArea.getWidth();
    
    // Calculate responsive panel widths
    auto actualChordPanelWidth = juce::jmin(CHORD_PANEL_WIDTH, 
                                           juce::jmax(300, availableWidth / 3));
    auto actualSettingsPanelWidth = juce::jmin(SETTINGS_PANEL_WIDTH,
                                              juce::jmax(280, availableWidth / 4));
    
    // Chord panel (left)
    auto chordPanelBounds = panelArea.removeFromLeft(actualChordPanelWidth);
    chordPanel->setBounds(chordPanelBounds);
    
    if (panelArea.getWidth() > PANEL_SPACING * 2 + actualSettingsPanelWidth + 200)
    {
        panelArea.removeFromLeft(PANEL_SPACING);
        
        // Settings panel (right)
        auto settingsPanelBounds = panelArea.removeFromRight(actualSettingsPanelWidth);
        settingsPanel->setBounds(settingsPanelBounds);
        panelArea.removeFromRight(PANEL_SPACING);
        
        // Structure panel (center - remaining space)
        structurePanel->setBounds(panelArea);
    }
    else
    {
        // Not enough space for 3 columns, stack settings and structure
        auto halfHeight = (PANEL_HEIGHT - PANEL_SPACING) / 2;
        
        auto topRow = panelArea.removeFromTop(halfHeight);
        structurePanel->setBounds(topRow);
        
        panelArea.removeFromTop(PANEL_SPACING);
        settingsPanel->setBounds(panelArea);
    }
    
    bounds.removeFromTop(PANEL_SPACING);
    
    // Pattern editor area (responsive height)
    auto patternHeight = juce::jmax(200, juce::jmin(PATTERN_EDITOR_HEIGHT, bounds.getHeight()));
    auto patternBounds = bounds.removeFromTop(patternHeight);
    patternEditor->setBounds(patternBounds);
}

void MainComponent::layoutVertically(juce::Rectangle<int> bounds)
{
    // Vertical stacking layout for smaller screens
    auto totalHeight = bounds.getHeight();
    
    // Divide available space proportionally
    auto chordPanelHeight = juce::jmax(200, totalHeight / 4);
    auto settingsHeight = juce::jmax(180, totalHeight / 5);
    auto structureHeight = juce::jmax(150, totalHeight / 5);
    auto patternHeight = juce::jmax(200, totalHeight - chordPanelHeight - settingsHeight - structureHeight - (PANEL_SPACING * 3));
    
    // Chord panel (top)
    auto chordBounds = bounds.removeFromTop(chordPanelHeight);
    chordPanel->setBounds(chordBounds);
    bounds.removeFromTop(PANEL_SPACING);
    
    // Settings panel
    auto settingsBounds = bounds.removeFromTop(settingsHeight);
    settingsPanel->setBounds(settingsBounds);
    bounds.removeFromTop(PANEL_SPACING);
    
    // Structure panel
    auto structureBounds = bounds.removeFromTop(structureHeight);
    structurePanel->setBounds(structureBounds);
    bounds.removeFromTop(PANEL_SPACING);
    
    // Pattern editor (remaining space)
    patternEditor->setBounds(bounds);
}

//==============================================================================
void MainComponent::timerCallback()
{
    if (isPlaying) {
        auto currentTime = juce::Time::getMillisecondCounter();
        
        if (currentTime >= lastStepTime + static_cast<juce::int64>(stepDurationMs)) {
            processCurrentStep();
            currentStep = (currentStep + 1) % 32;
            
            if (currentStep == 0 && !loopEnabled) {
                stopPlayback();
            }
            
            lastStepTime = currentTime;
            updatePlayheadDisplay();
        }
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    // Handle keyboard shortcuts matching Python implementation
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey) {
        // Play/Stop toggle
        onPlaybackStateChanged(!isPlaying);
        return true;
    }
    
    // TODO: Add more keyboard shortcuts when UI components are implemented
    // Arrow keys for navigation, etc.
    
    return false;
}

//==============================================================================
// Chord management callbacks
void MainComponent::onChordSelected(const juce::String& roman)
{
    selectedRoman = roman;
    DBG("Chord selected: " + roman);
    repaint(); // Update display
}

void MainComponent::onChordAdded()
{
    if (!selectedRoman.isEmpty()) {
        ChordData newChord(selectedRoman);
        chordProgression->addChord(newChord);
        
        DBG("Chord added: " + selectedRoman + " (Total: " + 
            juce::String(chordProgression->getChordCount()) + ")");
        
        updateStructurePanelState();
        updatePatternEditorState();
        selectedRoman.clear();
        updateChordPanelState();
    }
}

void MainComponent::onChordRemoved(int index)
{
    chordProgression->removeChord(index);
    DBG("Chord removed at index: " + juce::String(index));
    
    updateStructurePanelState();
    updatePatternEditorState();
}

void MainComponent::onChordModified(int index, const ChordData& newData)
{
    if (chordProgression->isValidChordIndex(index)) {
        // TODO: Implement chord modification in ChordProgression class
        DBG("Chord modified at index: " + juce::String(index));
        updateStructurePanelState();
    }
}

void MainComponent::onClearAllChords()
{
    chordProgression->clearChords();
    DBG("All chords cleared");
    
    updateStructurePanelState();
    updatePatternEditorState();
}

void MainComponent::onRandomizeChords()
{
    chordProgression->randomizeChords();
    DBG("Chords randomized");
    
    updateStructurePanelState();
}

//==============================================================================
// Playback control callbacks

void MainComponent::onTempoChanged(float newTempo)
{
    currentTempo = juce::jlimit(40.0f, 240.0f, newTempo);
    calculateStepDuration();
    DBG("Tempo changed to: " + juce::String(currentTempo, 1) + " BPM");
    repaint();
}

void MainComponent::onKeyChanged(const juce::String& newKey)
{
    currentKey = newKey;
    DBG("Key changed to: " + currentKey);
    repaint();
}

void MainComponent::onModeChanged(const juce::String& newMode)
{
    currentMode = newMode;
    DBG("Mode changed to: " + currentMode);
    repaint();
}

void MainComponent::onClickTrackChanged(bool enabled)
{
    clickTrackEnabled = enabled;
    DBG("Click track: " + juce::String(enabled ? "enabled" : "disabled"));
}

void MainComponent::onLoopChanged(bool enabled)
{
    loopEnabled = enabled;
    DBG("Loop: " + juce::String(enabled ? "enabled" : "disabled"));
}

void MainComponent::onPlaybackStateChanged(bool shouldPlay)
{
    if (shouldPlay && !isPlaying) {
        startPlayback();
    } else if (!shouldPlay && isPlaying) {
        stopPlayback();
    }
    
    // Update settings panel to reflect current state
    settingsPanel->setPlaybackState(isPlaying);
}

//==============================================================================
// Pattern editor callbacks
void MainComponent::onBlockAdded(const BlockData& block)
{
    chordProgression->addBlock(block);
    DBG("Block added: chord " + juce::String(block.chordIndex) + 
        " at step " + juce::String(block.startStep) + 
        " length " + juce::String(block.lengthSteps));
}

void MainComponent::onBlockRemoved(int blockIndex)
{
    chordProgression->removeBlock(blockIndex);
    DBG("Block removed at index: " + juce::String(blockIndex));
}

void MainComponent::onBlockModified(int blockIndex, const BlockData& newBlock)
{
    // TODO: Implement block modification
    DBG("Block modified at index: " + juce::String(blockIndex));
}

void MainComponent::onPatternRandomized(int minBlocks, int maxBlocks)
{
    chordProgression->randomizeBlocks(minBlocks, maxBlocks);
    DBG("Pattern randomized: " + juce::String(minBlocks) + "-" + juce::String(maxBlocks) + " blocks");
    updatePatternEditorState();
}

void MainComponent::onPatternCleared()
{
    chordProgression->clearBlocks();
    DBG("Pattern cleared");
    updatePatternEditorState();
}

//==============================================================================
// Export functionality
void MainComponent::onExportMidi()
{
    DBG("MIDI export requested");
    
    // Create file chooser
    fileChooser = std::make_unique<juce::FileChooser>("Export MIDI File",
                                                      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                                      "*.mid");
    
    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;
    
    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        
        if (file != juce::File{})
        {
            // Get the current blocks from pattern editor
            auto blocks = patternEditor->getAllBlocks();
            
            // Get the chord progression
            auto chords = chordProgression->getChords();
            
            // Export to MIDI
            bool success = MidiExporter::exportToFile(file, blocks, chords, 
                                                     currentTempo, currentKey, currentMode);
            
            if (success)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                      "Export Complete",
                                                      "MIDI file saved to:\n" + file.getFullPathName());
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                      "Export Failed",
                                                      "Failed to save MIDI file.");
            }
        }
    });
}

//==============================================================================
// Audio preview
void MainComponent::playChordPreview(int chordIndex)
{
    if (chordProgression->isValidChordIndex(chordIndex)) {
        const auto& chord = chordProgression->getChord(chordIndex);
        DBG("Playing chord preview: " + chord.roman);

        auto frequencies = MusicTheoryEngine::getChordFrequencies(
            chord.roman, currentKey, currentMode, chord);
        synthesizer->playChord(frequencies);
    }
}

void MainComponent::stopChordPreview()
{
    DBG("Stopping chord preview");
    synthesizer->stopAllNotes();
}

//==============================================================================
// Private methods
void MainComponent::setupLayout()
{
    // Layout is handled in resized()
}

void MainComponent::setupCallbacks()
{
    // Set up chord panel callbacks
    chordPanel->onChordSelected = [this](const juce::String& roman) {
        this->onChordSelected(roman);
    };
    
    chordPanel->onAddChord = [this]() {
        this->onChordAdded();
    };
    
    // Set up settings panel callbacks
    settingsPanel->onPlaybackStateChanged = [this](bool shouldPlay) {
        this->onPlaybackStateChanged(shouldPlay);
    };
    
    settingsPanel->onTempoChanged = [this](float newTempo) {
        this->onTempoChanged(newTempo);
    };
    
    settingsPanel->onKeyChanged = [this](const juce::String& newKey) {
        this->onKeyChanged(newKey);
    };
    
    settingsPanel->onModeChanged = [this](const juce::String& newMode) {
        this->onModeChanged(newMode);
    };
    
    settingsPanel->onLoopChanged = [this](bool enabled) {
        this->onLoopChanged(enabled);
    };
    
    settingsPanel->onClickTrackChanged = [this](bool enabled) {
        this->onClickTrackChanged(enabled);
    };
    
    // Set up structure panel callbacks
    structurePanel->onChordSelected = [this](int index) {
        DBG("Structure panel chord selected: " + juce::String(index));
        // TODO: Highlight selected chord, maybe play preview
    };
    
    structurePanel->onChordRemoved = [this](int index) {
        this->onChordRemoved(index);
    };
    
    structurePanel->onClearAll = [this]() {
        this->onClearAllChords();
    };
    
    structurePanel->onRandomizeProgression = [this]() {
        this->onRandomizeChords();
    };
    
    structurePanel->onExportProgression = [this]() {
        this->onExportMidi();
    };
    
    // Set up pattern editor callbacks
    patternEditor->onBlockAdded = [this](const BlockData& block) {
        this->onBlockAdded(block);
    };
    
    patternEditor->onBlockRemoved = [this](int blockIndex) {
        this->onBlockRemoved(blockIndex);
    };
    
    patternEditor->onBlockModified = [this](int blockIndex, const BlockData& newBlock) {
        this->onBlockModified(blockIndex, newBlock);
    };
    
    patternEditor->onPatternCleared = [this]() {
        this->onPatternCleared();
    };
    
    patternEditor->onPatternRandomized = [this](int minBlocks, int maxBlocks) {
        this->onPatternRandomized(minBlocks, maxBlocks);
    };
}

void MainComponent::setupKeyboardShortcuts()
{
    // Already handled in keyPressed()
}

void MainComponent::setupInitialState()
{
    calculateStepDuration();
}

void MainComponent::startPlayback()
{
    if (!isPlaying) {
        isPlaying = true;
        currentStep = 0;
        lastStepTime = juce::Time::getMillisecondCounter();

        if (!synthesizer->isRunning()) {
            synthesizer->start();
        }

        startTimer(static_cast<int>(stepDurationMs / 4)); // Check 4x per step for smooth timing
        
        DBG("Playback started");
        repaint();
    }
}

void MainComponent::stopPlayback()
{
    if (isPlaying) {
        isPlaying = false;
        stopTimer();
        stopAllNotes();
        
        DBG("Playback stopped");
        repaint();
    }
}

void MainComponent::processCurrentStep()
{
    DBG("Processing step: " + juce::String(currentStep + 1) + "/32");
    
    // Play notes for current step
    playNotesForStep(currentStep);
    
    // Play click track if enabled
    if (clickTrackEnabled) {
        playClickTrack(currentStep);
    }
    
    // Update progression state
    chordProgression->setCurrentStep(currentStep);
}

void MainComponent::updatePlayheadDisplay()
{
    // Update pattern editor playhead
    patternEditor->setCurrentStep(currentStep);
    repaint();
}

void MainComponent::calculateStepDuration()
{
    // 16th note duration in milliseconds
    stepDurationMs = (60.0 / currentTempo / 4.0) * 1000.0;
}

void MainComponent::playNotesForStep(int step)
{
    // Get all blocks active at this step
    auto activeBlocks = chordProgression->getBlocksAtStep(step);
    
    for (const auto& block : activeBlocks) {
        if (chordProgression->isValidChordIndex(block.chordIndex)) {
            const auto& baseChord = chordProgression->getChord(block.chordIndex);
            auto effectiveChord = block.getEffectiveChordData(baseChord);
            
            DBG("Playing chord at step " + juce::String(step + 1) + ": " + effectiveChord.roman);

            auto frequencies = MusicTheoryEngine::getChordFrequencies(
                effectiveChord.roman, currentKey, currentMode, effectiveChord);

            if (effectiveChord.hasArpeggiator()) {
                synthesizer->playArpeggiatedChord(frequencies, effectiveChord.arpMode,
                                                   effectiveChord.arpLength, currentTempo);
            } else {
                synthesizer->playChord(frequencies);
            }
        }
    }
}

void MainComponent::stopAllNotes()
{
    DBG("Stopping all notes");
    synthesizer->stopAllNotes();
}

void MainComponent::playClickTrack(int step)
{
    // Click on every step, accent on downbeats (every 4 steps)
    bool isDownbeat = (step % 4) == 0;

    synthesizer->playClick(isDownbeat);
    DBG("Click " + juce::String(isDownbeat ? "(accent)" : ""));
}

void MainComponent::updateChordPanelState()
{
    // Update chord panel selection state
    chordPanel->setSelectedChord(selectedRoman);
}

void MainComponent::updateStructurePanelState()
{
    // Update structure panel with current chord progression
    structurePanel->setChordProgression(chordProgression->getChords());
}

void MainComponent::updatePatternEditorState()
{
    // Update pattern editor with current blocks
    patternEditor->setBlocks(chordProgression->getBlocks());
    // Note: setChordCount method doesn't exist in PatternEditorComponent yet
}

void MainComponent::validateState()
{
    // Ensure all state is consistent
    if (currentStep >= 32) {
        currentStep = 0;
    }
    
    if (currentTempo < 40.0f || currentTempo > 240.0f) {
        currentTempo = 120.0f;
        calculateStepDuration();
    }
}

bool MainComponent::saveProject(const juce::File& file)
{
    // TODO: Implement project save
    DBG("Saving project to: " + file.getFullPathName());
    return false;
}

bool MainComponent::loadProject(const juce::File& file)
{
    // TODO: Implement project load
    DBG("Loading project from: " + file.getFullPathName());
    return false;
}

void MainComponent::showSaveDialog()
{
    // TODO: Implement save dialog
}

void MainComponent::showLoadDialog()
{
    // TODO: Implement load dialog
}

} // namespace ChordFoundry