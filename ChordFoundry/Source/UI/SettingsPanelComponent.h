#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * SettingsPanelComponent - Modern settings and playback control interface
 * 
 * Features:
 * - Playback controls (play/stop/loop)
 * - Tempo slider with real-time updates
 * - Key and mode selection dropdowns
 * - Click track toggle
 * - Volume and audio settings
 * - Keyboard shortcuts display
 */
class SettingsPanelComponent : public juce::Component,
                              public juce::Button::Listener,
                              public juce::Slider::Listener,
                              public juce::ComboBox::Listener
{
public:
    SettingsPanelComponent();
    ~SettingsPanelComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Component listeners
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    // Public API for updating from external state
    void setPlaybackState(bool isPlaying);
    void setTempo(float tempo);
    void setKey(const juce::String& key);
    void setMode(const juce::String& mode);
    void setLoopEnabled(bool enabled);
    void setClickTrackEnabled(bool enabled);
    
    // Callbacks
    std::function<void(bool)> onPlaybackStateChanged;
    std::function<void(float)> onTempoChanged;
    std::function<void(const juce::String&)> onKeyChanged;
    std::function<void(const juce::String&)> onModeChanged;
    std::function<void(bool)> onLoopChanged;
    std::function<void(bool)> onClickTrackChanged;
    
private:
    //==============================================================================
    // UI Components
    
    // Playback section
    std::unique_ptr<juce::Label> playbackLabel;
    std::unique_ptr<juce::TextButton> playButton;
    std::unique_ptr<juce::TextButton> stopButton;
    std::unique_ptr<juce::ToggleButton> loopButton;
    
    // Tempo section
    std::unique_ptr<juce::Label> tempoLabel;
    std::unique_ptr<juce::Slider> tempoSlider;
    std::unique_ptr<juce::Label> tempoValueLabel;
    
    // Musical settings section
    std::unique_ptr<juce::Label> musicalLabel;
    std::unique_ptr<juce::Label> keyLabel;
    std::unique_ptr<juce::ComboBox> keyComboBox;
    std::unique_ptr<juce::Label> modeLabel;
    std::unique_ptr<juce::ComboBox> modeComboBox;
    
    // Audio settings section
    std::unique_ptr<juce::Label> audioLabel;
    std::unique_ptr<juce::ToggleButton> clickTrackButton;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<juce::Label> volumeLabel;
    
    // Keyboard shortcuts section
    std::unique_ptr<juce::Label> shortcutsLabel;
    std::unique_ptr<juce::Label> shortcutsText;
    
    // State
    bool isPlaying = false;
    float currentTempo = 120.0f;
    juce::String currentKey = "C";
    juce::String currentMode = "Major";
    bool loopEnabled = false;
    bool clickTrackEnabled = false;
    
    //==============================================================================
    void setupUI();
    void setupPlaybackSection();
    void setupTempoSection();
    void setupMusicalSection();
    void setupAudioSection();
    void setupShortcutsSection();
    
    void updatePlaybackButtons();
    void updateTempoDisplay();
    
    // Layout helpers
    juce::Rectangle<int> createSection(juce::Rectangle<int>& bounds, const juce::String& title, int height);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanelComponent)
};

} // namespace ChordFoundry
