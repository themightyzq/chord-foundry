#include "SettingsPanelComponent.h"

namespace ChordFoundry {

//==============================================================================
SettingsPanelComponent::SettingsPanelComponent()
{
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
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingMD);
    
    // Playback section
    auto playbackBounds = createSection(bounds, "Playback", 120);
    
    if (playbackLabel)
        playbackLabel->setBounds(playbackBounds.removeFromTop(24));
    
    playbackBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    auto buttonHeight = ModernLookAndFeel::Metrics::buttonHeight;
    auto buttonRow1 = playbackBounds.removeFromTop(buttonHeight);
    auto buttonWidth = (buttonRow1.getWidth() - ModernLookAndFeel::Metrics::spacingSM) / 2;
    
    if (playButton)
        playButton->setBounds(buttonRow1.removeFromLeft(buttonWidth));
    buttonRow1.removeFromLeft(ModernLookAndFeel::Metrics::spacingSM);
    if (stopButton)
        stopButton->setBounds(buttonRow1);
    
    playbackBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    if (loopButton)
        loopButton->setBounds(playbackBounds.removeFromTop(28));
    
    // Tempo section
    auto tempoBounds = createSection(bounds, "Tempo", 80);
    
    if (tempoLabel)
        tempoLabel->setBounds(tempoBounds.removeFromTop(24));
    
    tempoBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    if (tempoSlider)
        tempoSlider->setBounds(tempoBounds.removeFromTop(24));
    
    if (tempoValueLabel)
        tempoValueLabel->setBounds(tempoBounds.removeFromTop(20));
    
    // Musical settings section
    auto musicalBounds = createSection(bounds, "Musical Settings", 100);
    
    if (musicalLabel)
        musicalLabel->setBounds(musicalBounds.removeFromTop(24));
    
    musicalBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    if (keyLabel)
        keyLabel->setBounds(musicalBounds.removeFromTop(20));
    if (keyComboBox)
        keyComboBox->setBounds(musicalBounds.removeFromTop(28));
    
    musicalBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    if (modeLabel)
        modeLabel->setBounds(musicalBounds.removeFromTop(20));
    if (modeComboBox)
        modeComboBox->setBounds(musicalBounds.removeFromTop(28));
    
    // Audio settings section
    auto audioBounds = createSection(bounds, "Audio Settings", 100);
    
    if (audioLabel)
        audioLabel->setBounds(audioBounds.removeFromTop(24));
    
    audioBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    if (clickTrackButton)
        clickTrackButton->setBounds(audioBounds.removeFromTop(28));
    
    audioBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
    
    if (volumeLabel)
        volumeLabel->setBounds(audioBounds.removeFromTop(20));
    if (volumeSlider)
        volumeSlider->setBounds(audioBounds.removeFromTop(24));
    
    // Keyboard shortcuts section (remaining space)
    if (bounds.getHeight() > 60)
    {
        auto shortcutsBounds = createSection(bounds, "Shortcuts", bounds.getHeight());
        
        if (shortcutsLabel)
            shortcutsLabel->setBounds(shortcutsBounds.removeFromTop(24));
        
        shortcutsBounds.removeFromTop(ModernLookAndFeel::Metrics::spacingSM);
        
        if (shortcutsText)
            shortcutsText->setBounds(shortcutsBounds);
    }
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
        // Handle volume changes (placeholder for now)
        // Could connect to audio system volume control
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

//==============================================================================
void SettingsPanelComponent::setupUI()
{
    setupPlaybackSection();
    setupTempoSection();
    setupMusicalSection();
    setupAudioSection();
    setupShortcutsSection();
}

void SettingsPanelComponent::setupPlaybackSection()
{
    // Section label
    playbackLabel = std::make_unique<juce::Label>("playback", "Playback");
    playbackLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    playbackLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(*playbackLabel);
    
    // Play button
    playButton = std::make_unique<juce::TextButton>("Play");
    playButton->addListener(this);
    playButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::success);
    playButton->setAccessible(true);
    playButton->setTitle("Start playback");
    playButton->setDescription("Start playing the chord progression - Shortcut: Space or Enter");
    addAndMakeVisible(*playButton);
    
    // Stop button
    stopButton = std::make_unique<juce::TextButton>("Stop");
    stopButton->addListener(this);
    stopButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::error);
    stopButton->setAccessible(true);
    stopButton->setTitle("Stop playback");
    addAndMakeVisible(*stopButton);
    
    // Loop button
    loopButton = std::make_unique<juce::ToggleButton>("Loop Playback");
    loopButton->addListener(this);
    loopButton->setAccessible(true);
    loopButton->setTitle("Enable loop playback");
    addAndMakeVisible(*loopButton);
    
    updatePlaybackButtons();
}

void SettingsPanelComponent::setupTempoSection()
{
    // Section label
    tempoLabel = std::make_unique<juce::Label>("tempo", "Tempo");
    tempoLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    tempoLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(*tempoLabel);
    
    // Tempo slider
    tempoSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    tempoSlider->setRange(40.0, 240.0, 1.0);
    tempoSlider->setValue(currentTempo);
    tempoSlider->addListener(this);
    tempoSlider->setAccessible(true);
    tempoSlider->setTitle("Tempo slider");
    tempoSlider->setDescription("Adjust playback tempo from 40 to 240 BPM");
    addAndMakeVisible(*tempoSlider);
    
    // Tempo value label with monospaced font for numeric display
    tempoValueLabel = std::make_unique<juce::Label>("tempoValue", "");
    tempoValueLabel->setFont(ModernLookAndFeel::Typography::getMonospacedFont());
    tempoValueLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    tempoValueLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*tempoValueLabel);
    
    updateTempoDisplay();
}

void SettingsPanelComponent::setupMusicalSection()
{
    // Section label
    musicalLabel = std::make_unique<juce::Label>("musical", "Musical Settings");
    musicalLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    musicalLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(*musicalLabel);
    
    // Key label and combo
    keyLabel = std::make_unique<juce::Label>("keyLabel", "Key:");
    keyLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    keyLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(*keyLabel);
    
    keyComboBox = std::make_unique<juce::ComboBox>("key");
    keyComboBox->addItemList({"C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"}, 1);
    keyComboBox->setText(currentKey, juce::dontSendNotification);
    keyComboBox->addListener(this);
    keyComboBox->setAccessible(true);
    keyComboBox->setTitle("Select musical key");
    addAndMakeVisible(*keyComboBox);
    
    // Mode label and combo
    modeLabel = std::make_unique<juce::Label>("modeLabel", "Mode:");
    modeLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    modeLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(*modeLabel);
    
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
    addAndMakeVisible(*modeComboBox);
}

void SettingsPanelComponent::setupAudioSection()
{
    // Section label
    audioLabel = std::make_unique<juce::Label>("audio", "Audio Settings");
    audioLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    audioLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(*audioLabel);
    
    // Click track toggle
    clickTrackButton = std::make_unique<juce::ToggleButton>("Click Track");
    clickTrackButton->addListener(this);
    clickTrackButton->setAccessible(true);
    clickTrackButton->setTitle("Enable click track (metronome)");
    addAndMakeVisible(*clickTrackButton);
    
    // Volume label and slider
    volumeLabel = std::make_unique<juce::Label>("volumeLabel", "Volume:");
    volumeLabel->setFont(ModernLookAndFeel::Typography::getCaptionFont());
    volumeLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(*volumeLabel);
    
    volumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0, 0.01);
    volumeSlider->setValue(0.7);
    volumeSlider->addListener(this);
    volumeSlider->setAccessible(true);
    volumeSlider->setTitle("Master volume");
    addAndMakeVisible(*volumeSlider);
}

void SettingsPanelComponent::setupShortcutsSection()
{
    // Section label
    shortcutsLabel = std::make_unique<juce::Label>("shortcuts", "Keyboard Shortcuts");
    shortcutsLabel->setFont(ModernLookAndFeel::Typography::getSubheaderFont());
    shortcutsLabel->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(*shortcutsLabel);
    
    // Shortcuts text
    juce::String shortcutsStr = "Space/Enter: Play/Stop\n";
    shortcutsStr += "Arrow Keys: Navigate\n";
    shortcutsStr += "Tab: Focus Next\n";
    shortcutsStr += "Shift+Tab: Focus Previous";
    
    shortcutsText = std::make_unique<juce::Label>("shortcutsText", shortcutsStr);
    shortcutsText->setFont(ModernLookAndFeel::Typography::getSmallFont());
    shortcutsText->setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    shortcutsText->setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(*shortcutsText);
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
