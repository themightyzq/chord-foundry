#include "SettingsPanelComponent.h"

namespace ChordFoundry {

//==============================================================================
SettingsPanelComponent::SettingsPanelComponent()
{
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(12);
    viewport.setTitle("Settings sections");
    addAndMakeVisible(viewport);

    setupUI();
    
    // Set accessibility properties
    setAccessible(true);
    setTitle("Settings and Playback Control Panel");
    setDescription("Control playback, tempo, key, mode, and audio settings");
}

//==============================================================================
void SettingsPanelComponent::paint(juce::Graphics& g)
{
    // Draw card background
    ModernLookAndFeel::drawCard(g, getLocalBounds(), true);
}

void SettingsPanelComponent::resized()
{
    viewport.setBounds(getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD, ModernLookAndFeel::Metrics::spacingSM));

    // Lay out at the full width; if that is taller than the viewport a scroll bar will take
    // some width, so lay out again at the narrower width.
    int width = viewport.getMaximumVisibleWidth();
    int height = layoutContent(width);

    if (height > viewport.getHeight())
    {
        width -= viewport.getScrollBarThickness();
        height = layoutContent(width);
    }

    content.setSize(width, height);
}

int SettingsPanelComponent::layoutContent(int width)
{
    const int sectionGap = 12;
    const int labelH = 28;
    const int small = ModernLookAndFeel::Metrics::spacingXS;
    const int gap = ModernLookAndFeel::Metrics::spacingSM;

    juce::Rectangle<int> bounds(0, 0, width, 100000);

    // Playback
    playbackLabel->setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(small);
    {
        auto row = bounds.removeFromTop(36);
        const int buttonWidth = (row.getWidth() - gap) / 2;
        playButton->setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(gap);
        stopButton->setBounds(row);
    }
    bounds.removeFromTop(small);
    loopButton->setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(sectionGap);

    // Tempo: slider with its value beside it
    tempoLabel->setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(small);
    {
        auto row = bounds.removeFromTop(28);
        tempoValueLabel->setBounds(row.removeFromRight(84));
        tempoSlider->setBounds(row);
    }
    bounds.removeFromTop(sectionGap);

    // Musical settings: key and mode side by side
    musicalLabel->setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(small);
    {
        auto labels = bounds.removeFromTop(18);
        auto combos = bounds.removeFromTop(28);
        const int columnWidth = (width - gap) / 2;

        keyLabel->setBounds(labels.removeFromLeft(columnWidth));
        labels.removeFromLeft(gap);
        modeLabel->setBounds(labels);

        keyComboBox->setBounds(combos.removeFromLeft(columnWidth));
        combos.removeFromLeft(gap);
        modeComboBox->setBounds(combos);
    }
    bounds.removeFromTop(sectionGap);

    // Audio settings
    audioLabel->setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(small);
    clickTrackButton->setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(small);
    {
        auto row = bounds.removeFromTop(28);
        volumeLabel->setBounds(row.removeFromLeft(64));
        volumeSlider->setBounds(row);
    }
    bounds.removeFromTop(sectionGap);

    // Keyboard shortcuts
    shortcutsLabel->setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(small);
    shortcutsText->setBounds(bounds.removeFromTop(72));

    return 100000 - bounds.getHeight();
}

//==============================================================================
void SettingsPanelComponent::buttonClicked(juce::Button* button)
{
    if (button == playButton.get())
    {
        setPlaybackState(true);
        if (onPlaybackStateChanged)
            onPlaybackStateChanged(true);
    }
    else if (button == stopButton.get())
    {
        setPlaybackState(false);
        if (onPlaybackStateChanged)
            onPlaybackStateChanged(false);
    }
    else if (button == loopButton.get())
    {
        loopEnabled = loopButton->getToggleState();
        if (onLoopChanged)
            onLoopChanged(loopEnabled);
    }
    else if (button == clickTrackButton.get())
    {
        clickTrackEnabled = clickTrackButton->getToggleState();
        if (onClickTrackChanged)
            onClickTrackChanged(clickTrackEnabled);
    }
}

void SettingsPanelComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == tempoSlider.get())
    {
        currentTempo = static_cast<float>(tempoSlider->getValue());
        updateTempoDisplay();
        
        if (onTempoChanged)
            onTempoChanged(currentTempo);
    }
    else if (slider == volumeSlider.get())
    {
        if (onVolumeChanged)
            onVolumeChanged(static_cast<float>(volumeSlider->getValue()));
    }
}

void SettingsPanelComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == keyComboBox.get())
    {
        currentKey = keyComboBox->getText();
        if (onKeyChanged)
            onKeyChanged(currentKey);
    }
    else if (comboBox == modeComboBox.get())
    {
        currentMode = modeComboBox->getText();
        if (onModeChanged)
            onModeChanged(currentMode);
    }
}

//==============================================================================
void SettingsPanelComponent::setPlaybackState(bool playing)
{
    if (isPlaying != playing)
    {
        isPlaying = playing;
        updatePlaybackButtons();
    }
}

void SettingsPanelComponent::setTempo(float tempo)
{
    if (currentTempo != tempo)
    {
        currentTempo = tempo;
        if (tempoSlider)
            tempoSlider->setValue(tempo, juce::dontSendNotification);
        updateTempoDisplay();
    }
}

void SettingsPanelComponent::setKey(const juce::String& key)
{
    if (currentKey != key)
    {
        currentKey = key;
        if (keyComboBox)
            keyComboBox->setText(key, juce::dontSendNotification);
    }
}

void SettingsPanelComponent::setMode(const juce::String& mode)
{
    if (currentMode != mode)
    {
        currentMode = mode;
        if (modeComboBox)
            modeComboBox->setText(mode, juce::dontSendNotification);
    }
}

void SettingsPanelComponent::setLoopEnabled(bool enabled)
{
    if (loopEnabled != enabled)
    {
        loopEnabled = enabled;
        if (loopButton)
            loopButton->setToggleState(enabled, juce::dontSendNotification);
    }
}

void SettingsPanelComponent::setClickTrackEnabled(bool enabled)
{
    if (clickTrackEnabled != enabled)
    {
        clickTrackEnabled = enabled;
        if (clickTrackButton)
            clickTrackButton->setToggleState(enabled, juce::dontSendNotification);
    }
}

void SettingsPanelComponent::setVolume(float volume)
{
    if (volumeSlider)
        volumeSlider->setValue(volume, juce::dontSendNotification);
}

//==============================================================================
void SettingsPanelComponent::setupUI()
{
    setupPlaybackSection();
    setupTempoSection();
    setupMusicalSection();
    setupAudioSection();
    setupShortcutsSection();
    setupTooltips();
}

void SettingsPanelComponent::setupTooltips()
{
    playButton->setTooltip("Start playing the pattern from step 1 (Space or Enter)");
    stopButton->setTooltip("Stop playback");
    loopButton->setTooltip("Repeat the 32-step pattern until stopped");
    tempoSlider->setTooltip("Tempo in beats per minute (40 to 240). Steps are 16th notes.");
    keyComboBox->setTooltip("The key the Roman numeral chords are built in");
    modeComboBox->setTooltip("The scale the chords are built from");
    clickTrackButton->setTooltip("Play a click on every step, accented on each beat, during playback");
    volumeSlider->setTooltip("Master volume of the built-in synth");
}

void SettingsPanelComponent::setupPlaybackSection()
{
    // Section label
    playbackLabel = std::make_unique<juce::Label>("playback", "Playback");
    playbackLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    playbackLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    content.addAndMakeVisible(*playbackLabel);
    
    // Play button
    playButton = std::make_unique<juce::TextButton>("Play");
    playButton->addListener(this);
    playButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::success);
    playButton->setAccessible(true);
    playButton->setTitle("Start playback");
    playButton->setDescription("Start playing the chord progression - Shortcut: Space or Enter");
    content.addAndMakeVisible(*playButton);
    
    // Stop button
    stopButton = std::make_unique<juce::TextButton>("Stop");
    stopButton->addListener(this);
    stopButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::error);
    stopButton->setAccessible(true);
    stopButton->setTitle("Stop playback");
    content.addAndMakeVisible(*stopButton);
    
    // Loop button
    loopButton = std::make_unique<juce::ToggleButton>("Loop Playback");
    loopButton->addListener(this);
    loopButton->setAccessible(true);
    loopButton->setTitle("Enable loop playback");
    content.addAndMakeVisible(*loopButton);
    
    updatePlaybackButtons();
}

void SettingsPanelComponent::setupTempoSection()
{
    // Section label
    tempoLabel = std::make_unique<juce::Label>("tempo", "Tempo");
    tempoLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    tempoLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    content.addAndMakeVisible(*tempoLabel);
    
    // Tempo slider
    tempoSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    tempoSlider->setRange(40.0, 240.0, 1.0);
    tempoSlider->setValue(currentTempo);
    tempoSlider->addListener(this);
    tempoSlider->setAccessible(true);
    tempoSlider->setTitle("Tempo slider");
    tempoSlider->setDescription("Adjust playback tempo from 40 to 240 BPM");
    content.addAndMakeVisible(*tempoSlider);
    
    // Tempo value label with monospaced font for numeric display
    tempoValueLabel = std::make_unique<juce::Label>("tempoValue", "");
    tempoValueLabel->setFont(ModernLookAndFeel::Typography::getMonospacedFont());
    tempoValueLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    tempoValueLabel->setJustificationType(juce::Justification::centred);
    content.addAndMakeVisible(*tempoValueLabel);
    
    updateTempoDisplay();
}

void SettingsPanelComponent::setupMusicalSection()
{
    // Section label
    musicalLabel = std::make_unique<juce::Label>("musical", "Musical Settings");
    musicalLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    musicalLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    content.addAndMakeVisible(*musicalLabel);
    
    // Key label and combo
    keyLabel = std::make_unique<juce::Label>("keyLabel", "Key:");
    keyLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    keyLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    content.addAndMakeVisible(*keyLabel);
    
    keyComboBox = std::make_unique<juce::ComboBox>("key");
    keyComboBox->addItemList({"C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"}, 1);
    keyComboBox->setText(currentKey, juce::dontSendNotification);
    keyComboBox->addListener(this);
    keyComboBox->setAccessible(true);
    keyComboBox->setTitle("Select musical key");
    content.addAndMakeVisible(*keyComboBox);
    
    // Mode label and combo
    modeLabel = std::make_unique<juce::Label>("modeLabel", "Mode:");
    modeLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    modeLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    content.addAndMakeVisible(*modeLabel);
    
    modeComboBox = std::make_unique<juce::ComboBox>("mode");
    modeComboBox->addItemList({
        "Major (Ionian)", "Dorian", "Phrygian", "Lydian", "Mixolydian", 
        "Minor (Aeolian)", "Locrian", "Gypsy Minor", "Harmonic Minor", 
        "Minor Pentatonic", "Whole Tone", "Tonic 2nds", "Tonic 3rds", "Tonic 4ths", "Tonic 6ths"
    }, 1);
    modeComboBox->setText(currentMode, juce::dontSendNotification);
    modeComboBox->addListener(this);
    modeComboBox->setAccessible(true);
    modeComboBox->setTitle("Select musical mode/scale");
    content.addAndMakeVisible(*modeComboBox);
}

void SettingsPanelComponent::setupAudioSection()
{
    // Section label
    audioLabel = std::make_unique<juce::Label>("audio", "Audio Settings");
    audioLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    audioLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    content.addAndMakeVisible(*audioLabel);
    
    // Click track toggle
    clickTrackButton = std::make_unique<juce::ToggleButton>("Click Track");
    clickTrackButton->addListener(this);
    clickTrackButton->setAccessible(true);
    clickTrackButton->setTitle("Enable click track (metronome)");
    content.addAndMakeVisible(*clickTrackButton);
    
    // Volume label and slider
    volumeLabel = std::make_unique<juce::Label>("volumeLabel", "Volume:");
    volumeLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    volumeLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    content.addAndMakeVisible(*volumeLabel);
    
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0, 0.01);
    volumeSlider->setValue(0.7);
    volumeSlider->addListener(this);
    volumeSlider->setAccessible(true);
    volumeSlider->setTitle("Master volume");
    content.addAndMakeVisible(*volumeSlider);
}

void SettingsPanelComponent::setupShortcutsSection()
{
    // Section label
    shortcutsLabel = std::make_unique<juce::Label>("shortcuts", "Keyboard Shortcuts");
    shortcutsLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    shortcutsLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    content.addAndMakeVisible(*shortcutsLabel);
    
    // Shortcuts text
    juce::String shortcutsStr = "Space/Enter: Play/Stop\n";
    shortcutsStr += "Cmd/Ctrl+N, O, S: New, Open, Save\n";
    shortcutsStr += "Cmd/Ctrl+Shift+S: Save As\n";
    shortcutsStr += "Tab / Shift+Tab: Move focus";
    
    shortcutsText = std::make_unique<juce::Label>("shortcutsText", shortcutsStr);
    shortcutsText->setFont(ModernLookAndFeel::Typography::getSmallFont());
    shortcutsText->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    shortcutsText->setJustificationType(juce::Justification::topLeft);
    content.addAndMakeVisible(*shortcutsText);
}

void SettingsPanelComponent::updatePlaybackButtons()
{
    if (playButton && stopButton)
    {
        playButton->setEnabled(!isPlaying);
        stopButton->setEnabled(isPlaying);
        
        if (isPlaying)
        {
            playButton->setButtonText("Playing...");
            playButton->setToggleState(true, juce::dontSendNotification);
        }
        else
        {
            playButton->setButtonText("Play");
            playButton->setToggleState(false, juce::dontSendNotification);
        }
    }
}

void SettingsPanelComponent::updateTempoDisplay()
{
    if (tempoValueLabel)
    {
        tempoValueLabel->setText(juce::String(currentTempo, 1) + " BPM", juce::dontSendNotification);
    }
}

juce::Rectangle<int> SettingsPanelComponent::createSection(juce::Rectangle<int>& bounds, 
                                                         const juce::String& title, int height)
{
    juce::ignoreUnused(title);
    
    auto section = bounds.removeFromTop(height);
    bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD); // Section spacing
    return section;
}

} // namespace ChordFoundry
