#include "MainComponent.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include "UI/ChordPanelComponent.h"
#include "UI/SettingsPanelComponent.h"
#include "UI/StructurePanelComponent.h"
#include "UI/PatternEditorComponent.h"
#include "UI/ChordModifierDialog.h"
#include "Audio/ChordSynthesizer.h"
#include "Audio/SequencerPatternBuilder.h"
#include "MusicTheory/MusicTheoryEngine.h"
#include "Export/MidiExporter.h"

namespace ChordFoundry {

//==============================================================================
// The scrolling surface the four panels sit on. It only forwards its size changes.
class MainComponent::ContentArea : public juce::Component
{
public:
    std::function<void()> onLayout;

    void resized() override
    {
        if (onLayout)
            onLayout();
    }
};

//==============================================================================
namespace {

// Contents of the audio settings dialog: JUCE's own device selector (output device,
// sample rate, buffer size) on the synth's device manager.
class AudioSettingsComponent : public juce::Component
{
public:
    AudioSettingsComponent(juce::AudioDeviceManager& manager, juce::LookAndFeel& lookAndFeel)
        : selector(manager, 0, 0, 2, 2, false, false, true, false)
    {
        setLookAndFeel(&lookAndFeel);
        addAndMakeVisible(selector);
        setSize(520, 380);
        setTitle("Audio settings");
    }

    ~AudioSettingsComponent() override { setLookAndFeel(nullptr); }

    void resized() override { selector.setBounds(getLocalBounds().reduced(12)); }

private:
    juce::AudioDeviceSelectorComponent selector;
};

void collectLayoutProblems(juce::Component& parent, juce::String& out, const juce::String& path)
{
    // A viewport's content is meant to extend past the viewport, so stop there.
    if (dynamic_cast<juce::Viewport*>(&parent) != nullptr)
        return;

    for (auto* child : parent.getChildren())
    {
        if (child == nullptr || ! child->isVisible())
            continue;

        const auto name = path + "/" + (child->getName().isNotEmpty() ? child->getName() : juce::String("?"));

        if (! parent.getLocalBounds().contains(child->getBounds()))
            out << "  CLIPPED " << name << " bounds " << child->getBounds().toString()
                << " not inside parent " << parent.getLocalBounds().toString() << "\n";

        const bool isControl = dynamic_cast<juce::Button*>(child) != nullptr
                            || dynamic_cast<juce::ComboBox*>(child) != nullptr
                            || dynamic_cast<juce::Slider*>(child) != nullptr;

        if (isControl && (child->getHeight() < 22 || child->getWidth() < 22))
            out << "  SMALL " << name << " " << child->getWidth() << "x" << child->getHeight() << "\n";

        collectLayoutProblems(*child, out, name);
    }
}

} // namespace

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize modern look and feel
    modernLookAndFeel = std::make_unique<ModernLookAndFeel>();
    setLookAndFeel(modernLookAndFeel.get());
    
    // Initialize core data structures
    chordProgression = std::make_unique<ChordProgression>();
    
    // The synthesizer is created here but its audio device is opened by startAudio().
    synthesizer = std::make_unique<ChordSynthesizer>();
    synthesizer->setMasterGain(masterVolume);
    synthesizer->setTempo(currentTempo);
    synthesizer->onDeviceStateChanged = [this]
    {
        if (isPlaying && ! synthesizer->hasAudioDevice())
        {
            stopPlayback();
            settingsPanel->setPlaybackState(false);
            showAudioProblem();
        }
        repaint();
    };
    
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
    
    // The panels sit on a scrolling content area.
    contentArea = std::make_unique<ContentArea>();
    contentArea->setName("content");
    contentArea->onLayout = [this] { layoutPanels(contentArea->getLocalBounds()); };
    contentArea->addAndMakeVisible(*chordPanel);
    contentArea->addAndMakeVisible(*settingsPanel);
    contentArea->addAndMakeVisible(*structurePanel);
    contentArea->addAndMakeVisible(*patternEditor);

    contentViewport.setViewedComponent(contentArea.get(), false);
    contentViewport.setScrollBarsShown(true, true);
    contentViewport.setScrollBarThickness(12);
    contentViewport.setTitle("Main content");
    contentViewport.setWantsKeyboardFocus(false);
    addAndMakeVisible(contentViewport);
    
    setupToolbar();
    setupCallbacks();
    setupCommands();
    
    // Set up keyboard listener and accessibility
    addKeyListener(this);
    addKeyListener(commandManager.getKeyMappings());
    setWantsKeyboardFocus(true);
    setAccessible(true);
    setTitle("Chord Foundry - Professional chord progression and sequencing tool");
    setDescription("Main application window with chord selection, structure editing, and pattern sequencing");

    settingsPanel->setVolume(masterVolume);
    
    // Set initial size: comfortably above the minimum content size so the layout has room.
    setSize(1280, 820);

    savedSnapshot = getCurrentSnapshot();
    refreshSequencerPattern();
}

MainComponent::~MainComponent()
{
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr);
   #endif

    if (audioSettingsWindow != nullptr)
        delete audioSettingsWindow.getComponent();

    if (modifierWindow != nullptr)
        delete modifierWindow.getComponent();

    stopTimer();
    if (isPlaying)
        stopPlayback();

    if (synthesizer)
    {
        synthesizer->onDeviceStateChanged = nullptr;
        synthesizer->stop();
    }

    // Children must not outlive the look and feel they use.
    contentViewport.setViewedComponent(nullptr, false);
    contentArea.reset();
    chordPanel.reset();
    settingsPanel.reset();
    structurePanel.reset();
    patternEditor.reset();
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::startAudio()
{
    if (synthesizer->start())
        return;

    // Tell the user once the window is up, not from inside the constructor.
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this)]
    {
        if (safe != nullptr)
            safe->showAudioProblem();
    });
}

void MainComponent::showAudioProblem()
{
    if (audioProblemShowing)
        return;

    const auto problem = synthesizer->getDeviceProblem();
    if (problem.isEmpty())
        return;

    audioProblemShowing = true;

    const auto options = juce::MessageBoxOptions()
                             .withIconType(juce::MessageBoxIconType::WarningIcon)
                             .withTitle("No audio output")
                             .withMessage(problem)
                             .withButton("Audio Settings")
                             .withButton("Try Again")
                             .withButton("Dismiss")
                             .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [safe = juce::Component::SafePointer<MainComponent>(this)](int result)
    {
        if (safe == nullptr)
            return;

        safe->audioProblemShowing = false;

        if (result == 1)
        {
            safe->showAudioSettings();
        }
        else if (result == 2)
        {
            if (! safe->synthesizer->start())
                safe->showAudioProblem();
            safe->repaint();
        }
    });
}

void MainComponent::showAudioSettings()
{
    if (audioSettingsWindow != nullptr)
    {
        audioSettingsWindow->toFront(true);
        return;
    }

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(new AudioSettingsComponent(synthesizer->getDeviceManager(), *modernLookAndFeel));
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = ModernLookAndFeel::Colors::surface;
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    audioSettingsWindow = options.launchAsync();
}

//==============================================================================
juce::Rectangle<int> MainComponent::getHeaderBounds() const
{
    return getLocalBounds().withTrimmedTop(getMenuBarHeight()).removeFromTop(HEADER_HEIGHT);
}

int MainComponent::getMenuBarHeight() const
{
    return menuBar != nullptr ? MENU_BAR_HEIGHT : 0;
}

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
    auto headerBounds = getHeaderBounds();
    
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
    
    // The toolbar buttons sit at the left and right edges; the title is centred between them.
    auto titleArea = getHeaderBounds().withTrimmedLeft(saveAsButton.getRight() + WINDOW_PADDING)
                                      .withTrimmedRight(getWidth() - audioButton.getX() + WINDOW_PADDING);
    auto titleBounds = titleArea.removeFromTop(38);
    
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(ModernLookAndFeel::Typography::getHeaderFont().withHeight(26.0f));
    g.drawText("Chord Foundry", titleBounds.withTrimmedTop(4), juce::Justification::centred);
    
    // Subtitle
    g.setColour(ModernLookAndFeel::Colors::textSecondary.brighter(0.3f));
    g.setFont(ModernLookAndFeel::Typography::getCaptionFont());
    g.drawText("Professional chord progression and sequencing tool", 
               titleArea, juce::Justification::centredTop);
    
    // Enhanced status bar with gradient background
    auto statusBounds = getLocalBounds().removeFromBottom(STATUS_BAR_HEIGHT);
    
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
    
    if (isPlaying)
        stateText += " | Playing (Step " + juce::String(juce::jmax(0, currentStep) + 1) + "/32)";
    
    // Enhanced playback indicator with glow effect
    if (isPlaying) {
        auto indicatorBounds = textBounds.removeFromRight(60);
        auto dotBounds = indicatorBounds.removeFromLeft(12).withSizeKeepingCentre(12, 12).toFloat();
        
        g.setColour(ModernLookAndFeel::Colors::success.withAlpha(0.3f));
        g.fillEllipse(dotBounds.expanded(2.0f));
        
        g.setColour(ModernLookAndFeel::Colors::success);
        g.fillEllipse(dotBounds);
        
        g.setColour(ModernLookAndFeel::Colors::success.brighter(0.2f));
        g.setFont(ModernLookAndFeel::Typography::getSmallFont().boldened());
        g.drawText("LIVE", indicatorBounds.withTrimmedLeft(4), juce::Justification::centredLeft);
        g.setFont(ModernLookAndFeel::Typography::getSmallFont());
    }

    // A missing audio device is shown here as well as in the alert, so it is not forgotten.
    if (synthesizer != nullptr && synthesizer->isRunning() && ! synthesizer->hasAudioDevice())
    {
        auto audioText = textBounds.removeFromRight(260);
        g.setColour(ModernLookAndFeel::Colors::error.brighter(0.3f)); // 5:1 against the status bar
        g.setFont(ModernLookAndFeel::Typography::getSmallFont().boldened());
        g.drawText("No audio output - open Audio Settings", audioText, juce::Justification::centredRight);
        g.setFont(ModernLookAndFeel::Typography::getSmallFont());
    }

    g.setColour(isPlaying ? ModernLookAndFeel::Colors::success.brighter(0.2f)
                          : ModernLookAndFeel::Colors::textSecondary.brighter(0.1f));
    g.drawText(stateText, textBounds, juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    if (menuBar != nullptr)
        menuBar->setBounds(bounds.removeFromTop(MENU_BAR_HEIGHT));

    // Header toolbar: project buttons at the left, audio settings at the right.
    auto header = bounds.removeFromTop(HEADER_HEIGHT).reduced(WINDOW_PADDING, 0);
    constexpr int buttonH = 30;
    auto row = header.withSizeKeepingCentre(header.getWidth(), buttonH);

    newButton.setBounds(row.removeFromLeft(56));
    row.removeFromLeft(6);
    openButton.setBounds(row.removeFromLeft(60));
    row.removeFromLeft(6);
    saveButton.setBounds(row.removeFromLeft(56));
    row.removeFromLeft(6);
    saveAsButton.setBounds(row.removeFromLeft(72));
    audioButton.setBounds(row.removeFromRight(124));

    bounds.removeFromBottom(STATUS_BAR_HEIGHT);
    contentViewport.setBounds(bounds);
    updateContentSize();
    repaint();
}

void MainComponent::updateContentSize()
{
    // Fill the viewport; if it is smaller than the layout needs, keep the layout size and let
    // the viewport scroll. Leave room for a scroll bar only when that bar will be shown.
    const int bar = contentViewport.getScrollBarThickness();
    const int visibleW = contentViewport.getWidth();
    const int visibleH = contentViewport.getHeight();

    const bool needsVertical = visibleH < MIN_CONTENT_HEIGHT;
    const bool needsHorizontal = visibleW - (needsVertical ? bar : 0) < MIN_CONTENT_WIDTH;

    const int w = juce::jmax(MIN_CONTENT_WIDTH, visibleW - ((needsVertical || visibleH - (needsHorizontal ? bar : 0) < MIN_CONTENT_HEIGHT) ? bar : 0));
    const int h = juce::jmax(MIN_CONTENT_HEIGHT, visibleH - (needsHorizontal ? bar : 0));

    contentArea->setSize(w, h);
}

void MainComponent::layoutPanels(juce::Rectangle<int> area)
{
    auto bounds = area.reduced(WINDOW_PADDING);

    // Three columns on top, the pattern editor below. The top row takes about two thirds of the
    // height, the pattern editor the rest, each within sensible limits.
    const int usableHeight = bounds.getHeight() - PANEL_SPACING;
    const int patternHeight = juce::jlimit(190, 320, juce::roundToInt(usableHeight * 0.32f));
    const int topHeight = usableHeight - patternHeight;

    auto topRow = bounds.removeFromTop(topHeight);
    bounds.removeFromTop(PANEL_SPACING);
    patternEditor->setBounds(bounds);

    const int chordWidth = juce::jlimit(300, 400, juce::roundToInt(topRow.getWidth() * 0.30f));
    const int settingsWidth = juce::jlimit(270, 320, juce::roundToInt(topRow.getWidth() * 0.26f));

    chordPanel->setBounds(topRow.removeFromLeft(chordWidth));
    topRow.removeFromLeft(PANEL_SPACING);
    settingsPanel->setBounds(topRow.removeFromRight(settingsWidth));
    topRow.removeFromRight(PANEL_SPACING);
    structurePanel->setBounds(topRow);
}

juce::String MainComponent::describeLayout() const
{
    juce::String out;
    out << "window " << getWidth() << "x" << getHeight() << "\n";
    out << "viewport " << contentViewport.getBounds().toString()
        << " content " << contentArea->getWidth() << "x" << contentArea->getHeight()
        << (contentArea->getHeight() > contentViewport.getMaximumVisibleHeight()
            || contentArea->getWidth() > contentViewport.getMaximumVisibleWidth() ? " (scrolls)" : " (fits)") << "\n";

    const auto line = [&out](const char* name, const juce::Component& c)
    {
        out << "  " << name << " " << c.getBounds().toString() << "\n";
    };
    line("chordPanel", *chordPanel);
    line("structurePanel", *structurePanel);
    line("settingsPanel", *settingsPanel);
    line("patternEditor", *patternEditor);

    out << "problems:\n";
    const auto before = out.length();
    collectLayoutProblems(*const_cast<MainComponent*>(this), out, "main");
    if (out.length() == before)
        out << "  none\n";

    return out;
}

//==============================================================================
void MainComponent::timerCallback()
{
    if (! isPlaying)
        return;

    // The audio engine counts the steps. This only follows it for the display.
    const int step = synthesizer->getSequencerStep();
    if (step != currentStep)
    {
        currentStep = step;
        if (currentStep >= 0)
            chordProgression->setCurrentStep(currentStep);
        updatePlayheadDisplay();
    }

    // A pattern that is not looping ends by itself on the audio thread.
    if (synthesizer->getSequencerFinishedCount() != finishedCountAtStart)
    {
        stopPlayback();
        settingsPanel->setPlaybackState(false);
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

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

        // Select the new chord: the Modifiers button and the pattern grid then work on it.
        const int newIndex = chordProgression->getChordCount() - 1;
        structurePanel->setCurrentChordIndex(newIndex);
        patternEditor->setSelectedChordIndex(newIndex);
        selectedRoman.clear();
        updateChordPanelState();
        modelChanged();
    }
}

void MainComponent::onChordRemoved(int index)
{
    chordProgression->removeChord(index);
    DBG("Chord removed at index: " + juce::String(index));
    
    updateStructurePanelState();
    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onChordModified(int index, const ChordData& newData)
{
    // The progression is the source of truth: the change goes through it, then the panels,
    // the playing pattern and the unsaved-changes state follow. (The app has no undo stack.)
    if (chordProgression->updateChord(index, newData)) {
        DBG("Chord modified at index: " + juce::String(index));
        updateStructurePanelState();
        modelChanged();
    }
}

void MainComponent::showChordModifiers(int index)
{
    if (! chordProgression->isValidChordIndex(index))
        return;

    if (modifierWindow != nullptr)
    {
        modifierWindow->toFront(true);
        return;
    }

    auto* dialog = new ChordModifierDialog(chordProgression->getChord(index));

    dialog->onPreview = [this](const ChordData& chord)
    {
        const auto frequencies = MusicTheoryEngine::getChordFrequencies(chord.roman, currentKey, currentMode, chord);
        synthesizer->playChord(frequencies);
    };

    dialog->onApply = [this, index](const ChordData& edited)
    {
        onChordModified(index, edited);
    };

    dialog->onClosed = [this]
    {
        stopChordPreview();
    };

    modifierWindow = dialog;
    dialog->launch(this);
}

void MainComponent::onClearAllChords()
{
    chordProgression->clearChords();
    DBG("All chords cleared");
    
    updateStructurePanelState();
    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onRandomizeChords()
{
    chordProgression->randomizeChords();
    DBG("Chords randomized");
    
    updateStructurePanelState();
    modelChanged();
}

//==============================================================================
// Playback control callbacks

void MainComponent::onTempoChanged(float newTempo)
{
    currentTempo = juce::jlimit(40.0f, 240.0f, newTempo);
    synthesizer->setTempo(currentTempo);
    DBG("Tempo changed to: " + juce::String(currentTempo, 1) + " BPM");
    modelChanged();
}

void MainComponent::onKeyChanged(const juce::String& newKey)
{
    currentKey = newKey;
    DBG("Key changed to: " + currentKey);
    modelChanged();
}

void MainComponent::onModeChanged(const juce::String& newMode)
{
    currentMode = newMode;
    DBG("Mode changed to: " + currentMode);
    modelChanged();
}

void MainComponent::onClickTrackChanged(bool enabled)
{
    clickTrackEnabled = enabled;
    synthesizer->setSequencerClickEnabled(enabled);
    DBG("Click track: " + juce::String(enabled ? "enabled" : "disabled"));
    modelChanged();
}

void MainComponent::onLoopChanged(bool enabled)
{
    loopEnabled = enabled;
    synthesizer->setLoop(enabled);
    DBG("Loop: " + juce::String(enabled ? "enabled" : "disabled"));
    modelChanged();
}

void MainComponent::onVolumeChanged(float newVolume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, newVolume);
    synthesizer->setMasterGain(masterVolume);
    modelChanged();
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

    // The progression is the source of truth; show exactly what it accepted.
    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onBlockRemoved(int blockIndex)
{
    chordProgression->removeBlock(blockIndex);
    DBG("Block removed at index: " + juce::String(blockIndex));

    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onBlockModified(int blockIndex, const BlockData& newBlock)
{
    chordProgression->replaceBlock(blockIndex, newBlock);
    DBG("Block modified at index: " + juce::String(blockIndex));

    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onPatternRandomized(int minBlocks, int maxBlocks)
{
    chordProgression->randomizeBlocks(minBlocks, maxBlocks);
    DBG("Pattern randomized: " + juce::String(minBlocks) + "-" + juce::String(maxBlocks) + " blocks");
    updatePatternEditorState();
    modelChanged();
}

void MainComponent::onPatternCleared()
{
    chordProgression->clearBlocks();
    DBG("Pattern cleared");
    updatePatternEditorState();
    modelChanged();
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
            // Export what is actually in the project, not a copy held by the editor.
            const auto& blocks = chordProgression->getBlocks();
            const auto& chords = chordProgression->getChords();
            
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
void MainComponent::setupToolbar()
{
    const auto setup = [this](juce::TextButton& button, const juce::String& tip, const juce::String& title)
    {
        button.setTooltip(tip);
        button.setTitle(title);
        button.setAccessible(true);
        button.setWantsKeyboardFocus(true);
        addAndMakeVisible(button);
    };

    setup(newButton, "Start a new, empty project (Cmd/Ctrl+N)", "New project");
    setup(openButton, "Open a saved project (Cmd/Ctrl+O)", "Open project");
    setup(saveButton, "Save the project (Cmd/Ctrl+S)", "Save project");
    setup(saveAsButton, "Save the project under a new name (Cmd/Ctrl+Shift+S)", "Save project as");
    setup(audioButton, "Choose the audio output device, sample rate and buffer size", "Audio settings");

    newButton.onClick = [this] { newProject(); };
    openButton.onClick = [this] { openProject(); };
    saveButton.onClick = [this] { saveProject(); };
    saveAsButton.onClick = [this] { saveProjectAs(); };
    audioButton.onClick = [this] { showAudioSettings(); };
}

void MainComponent::setupCommands()
{
    commandManager.registerAllCommandsForTarget(this);
    setApplicationCommandManagerToWatch(&commandManager);

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(this);
   #else
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    menuBar->setTitle("Menu bar");
    addAndMakeVisible(*menuBar);
   #endif
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

    settingsPanel->onVolumeChanged = [this](float volume) {
        this->onVolumeChanged(volume);
    };
    
    // Set up structure panel callbacks
    structurePanel->onChordSelected = [this](int index) {
        DBG("Structure panel chord selected: " + juce::String(index));
        // Blocks drawn on the grid from now on use this chord.
        patternEditor->setSelectedChordIndex(index);
    };
    
    structurePanel->onModifyChord = [this](int index) {
        this->showChordModifiers(index);
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

//==============================================================================
// Playback. The audio engine runs the steps; these just start and stop it.
void MainComponent::startPlayback()
{
    if (isPlaying)
        return;

    // The device may have failed at launch or been unplugged since; try again, and if it
    // still is not there say so instead of "playing" in silence.
    if (! synthesizer->start())
    {
        showAudioProblem();
        return;
    }

    refreshSequencerPattern();
    synthesizer->setTempo(currentTempo);
    synthesizer->setLoop(loopEnabled);
    synthesizer->setSequencerClickEnabled(clickTrackEnabled);

    finishedCountAtStart = synthesizer->getSequencerFinishedCount();
    currentStep = -1;
    isPlaying = true;
    synthesizer->startSequencer();
    startTimerHz(30);

    DBG("Playback started");
    repaint();
}

void MainComponent::stopPlayback()
{
    if (isPlaying) {
        isPlaying = false;
        stopTimer();
        synthesizer->stopSequencer();
        currentStep = -1;
        patternEditor->setCurrentStep(-1);
        
        DBG("Playback stopped");
        repaint();
    }
}

void MainComponent::updatePlayheadDisplay()
{
    // Update pattern editor playhead
    patternEditor->setCurrentStep(currentStep);
    repaint();
}

void MainComponent::stopAllNotes()
{
    DBG("Stopping all notes");
    synthesizer->stopAllNotes();
}

void MainComponent::refreshSequencerPattern()
{
    // Pushes the current chords, blocks, key and mode to the audio engine. It takes over at the
    // next step, and chords that did not change keep sounding.
    synthesizer->setSequencerPattern(
        SequencerPatternBuilder::build(*chordProgression, currentKey, currentMode));
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
    patternEditor->setChordCount(chordProgression->getChordCount());
}

void MainComponent::modelChanged()
{
    refreshSequencerPattern();
    updateWindowTitle();
    repaint();
}

//==============================================================================
// Project files
ProjectSettings MainComponent::getCurrentSettings() const
{
    ProjectSettings settings;
    settings.tempo = currentTempo;
    settings.key = currentKey;
    settings.mode = currentMode;
    settings.loop = loopEnabled;
    settings.clickTrack = clickTrackEnabled;
    settings.masterVolume = masterVolume;
    return settings;
}

juce::String MainComponent::getCurrentSnapshot() const
{
    return ProjectFile::toXmlString(getCurrentSettings(), *chordProgression);
}

bool MainComponent::hasUnsavedChanges() const
{
    return getCurrentSnapshot() != savedSnapshot;
}

int MainComponent::getChordCountForCheck() const
{
    return chordProgression->getChordCount();
}

ChordData MainComponent::getChordForCheck(int index) const
{
    return chordProgression->getChord(index);
}

juce::String MainComponent::getProjectName() const
{
    return currentProjectFile == juce::File() ? juce::String("Untitled")
                                              : currentProjectFile.getFileName();
}

void MainComponent::updateWindowTitle()
{
    if (auto* window = getTopLevelComponent())
        if (window != this)
            window->setName("Chord Foundry - " + getProjectName() + (hasUnsavedChanges() ? " *" : ""));
}

void MainComponent::applyProject(const ProjectSettings& settings)
{
    if (isPlaying)
        stopPlayback();

    currentTempo = juce::jlimit(40.0f, 240.0f, settings.tempo);
    currentKey = settings.key;
    currentMode = settings.mode;
    loopEnabled = settings.loop;
    clickTrackEnabled = settings.clickTrack;
    masterVolume = settings.masterVolume;

    synthesizer->setTempo(currentTempo);
    synthesizer->setLoop(loopEnabled);
    synthesizer->setSequencerClickEnabled(clickTrackEnabled);
    synthesizer->setMasterGain(masterVolume);

    settingsPanel->setTempo(currentTempo);
    settingsPanel->setKey(currentKey);
    settingsPanel->setMode(currentMode);
    settingsPanel->setLoopEnabled(loopEnabled);
    settingsPanel->setClickTrackEnabled(clickTrackEnabled);
    settingsPanel->setVolume(masterVolume);
    settingsPanel->setPlaybackState(false);

    selectedRoman.clear();
    updateChordPanelState();
    updateStructurePanelState();
    updatePatternEditorState();
    patternEditor->setSelectedChordIndex(0);
    patternEditor->setCurrentStep(-1);
    refreshSequencerPattern();
    repaint();
}

bool MainComponent::writeProjectFile(const juce::File& file)
{
    const auto result = ProjectFile::save(file, getCurrentSettings(), *chordProgression);

    if (result.failed())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Could not save the project",
                                               result.getErrorMessage(), "OK", this);
        return false;
    }

    currentProjectFile = file;
    savedSnapshot = getCurrentSnapshot();
    updateWindowTitle();
    return true;
}

bool MainComponent::openProjectFile(const juce::File& file)
{
    ProjectSettings settings;
    ChordProgression loaded;
    const auto result = ProjectFile::load(file, settings, loaded);

    if (result.failed())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Could not open the project",
                                               result.getErrorMessage(), "OK", this);
        return false;
    }

    chordProgression->fromValueTree(loaded.toValueTree());
    applyProject(settings);

    currentProjectFile = file;
    savedSnapshot = getCurrentSnapshot();
    updateWindowTitle();
    return true;
}

void MainComponent::confirmDiscardChanges(std::function<void(bool)> proceed)
{
    if (! hasUnsavedChanges())
    {
        proceed(true);
        return;
    }

    const auto options = juce::MessageBoxOptions()
                             .withIconType(juce::MessageBoxIconType::QuestionIcon)
                             .withTitle("Save changes?")
                             .withMessage("\"" + getProjectName() + "\" has changes that have not been saved. "
                                          "Save them before you continue?")
                             .withButton("Save")
                             .withButton("Don't Save")
                             .withButton("Cancel")
                             .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [safe = juce::Component::SafePointer<MainComponent>(this), proceed](int result)
    {
        if (safe == nullptr)
            return;

        if (result == 1)
            safe->saveProject([proceed](bool saved) { proceed(saved); });
        else if (result == 2)
            proceed(true);
        else
            proceed(false);
    });
}

void MainComponent::newProject()
{
    confirmDiscardChanges([safe = juce::Component::SafePointer<MainComponent>(this)](bool proceed)
    {
        if (safe == nullptr || ! proceed)
            return;

        safe->chordProgression->clearChords();
        safe->applyProject(ProjectSettings());
        safe->currentProjectFile = juce::File();
        safe->savedSnapshot = safe->getCurrentSnapshot();
        safe->updateWindowTitle();
    });
}

void MainComponent::openProject()
{
    confirmDiscardChanges([safe = juce::Component::SafePointer<MainComponent>(this)](bool proceed)
    {
        if (safe == nullptr || ! proceed)
            return;

        const auto start = safe->currentProjectFile != juce::File()
                               ? safe->currentProjectFile.getParentDirectory()
                               : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

        safe->fileChooser = std::make_unique<juce::FileChooser>("Open Project", start, ProjectFile::fileWildcard);

        safe->fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                       [safe](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (safe != nullptr && file != juce::File())
                safe->openProjectFile(file);
        });
    });
}

void MainComponent::saveProject(std::function<void(bool)> done)
{
    if (currentProjectFile == juce::File())
    {
        saveProjectAs(std::move(done));
        return;
    }

    const bool ok = writeProjectFile(currentProjectFile);
    if (done)
        done(ok);
}

void MainComponent::saveProjectAs(std::function<void(bool)> done)
{
    const auto start = currentProjectFile != juce::File()
                           ? currentProjectFile
                           : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Untitled" + juce::String(ProjectFile::fileExtension));

    fileChooser = std::make_unique<juce::FileChooser>("Save Project", start, ProjectFile::fileWildcard);

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [safe = juce::Component::SafePointer<MainComponent>(this), done](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();

        if (safe == nullptr)
            return;

        if (file == juce::File())
        {
            if (done)
                done(false);
            return;
        }

        if (file.getFileExtension().isEmpty())
            file = file.withFileExtension(ProjectFile::fileExtension);

        const bool ok = safe->writeProjectFile(file);
        if (done)
            done(ok);
    });
}

//==============================================================================
// Menu bar and commands
juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Audio" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (menuIndex == 0)
    {
        menu.addCommandItem(&commandManager, cmdNewProject);
        menu.addCommandItem(&commandManager, cmdOpenProject);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, cmdSaveProject);
        menu.addCommandItem(&commandManager, cmdSaveProjectAs);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, cmdExportMidi);
    }
    else if (menuIndex == 1)
    {
        menu.addCommandItem(&commandManager, cmdAudioSettings);
    }

    return menu;
}

void MainComponent::menuItemSelected(int, int) {}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({ cmdNewProject, cmdOpenProject, cmdSaveProject, cmdSaveProjectAs,
                        cmdExportMidi, cmdAudioSettings });
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto cmdShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

    switch (commandID)
    {
        case cmdNewProject:
            result.setInfo("New Project", "Start a new, empty project", "File", 0);
            result.addDefaultKeypress('n', cmd);
            break;
        case cmdOpenProject:
            result.setInfo("Open Project...", "Open a saved project", "File", 0);
            result.addDefaultKeypress('o', cmd);
            break;
        case cmdSaveProject:
            result.setInfo("Save Project", "Save the project", "File", 0);
            result.addDefaultKeypress('s', cmd);
            break;
        case cmdSaveProjectAs:
            result.setInfo("Save Project As...", "Save the project under a new name", "File", 0);
            result.addDefaultKeypress('s', cmdShift);
            break;
        case cmdExportMidi:
            result.setInfo("Export MIDI...", "Export the pattern as a MIDI file", "File", 0);
            result.addDefaultKeypress('e', cmd);
            break;
        case cmdAudioSettings:
            result.setInfo("Audio Settings...", "Choose the audio output device", "Audio", 0);
            result.addDefaultKeypress(',', cmd);
            break;
        default:
            break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case cmdNewProject:     newProject(); return true;
        case cmdOpenProject:    openProject(); return true;
        case cmdSaveProject:    saveProject(); return true;
        case cmdSaveProjectAs:  saveProjectAs(); return true;
        case cmdExportMidi:     onExportMidi(); return true;
        case cmdAudioSettings:  showAudioSettings(); return true;
        default:                break;
    }

    return false;
}

} // namespace ChordFoundry
