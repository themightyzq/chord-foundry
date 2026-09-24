#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Data/ChordProgression.h"
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * ChordModifierDialog - Modern dialog for editing chord properties
 * 
 * Features:
 * - Extension selection (7, 9, 11, add9, sus2, sus4, etc.)
 * - Inversion options (root, 1st, 2nd, 3rd)
 * - Voicing selection (close, open, drop2, drop3, custom)
 * - Arpeggiator settings (mode, length, speed)
 * - Custom voicing editor (note count, position, spread)
 */
class ChordModifierDialog : public juce::DialogWindow,
                           public juce::Button::Listener,
                           public juce::ComboBox::Listener,
                           public juce::Slider::Listener
{
public:
    ChordModifierDialog(const ChordData& chord);
    ~ChordModifierDialog() override = default;
    
    ChordData getModifiedChord() const { return modifiedChord; }
    
    // Component overrides
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Callback for changes
    std::function<void()> onPreviewChord;
    
private:
    void updateChordFromUI();
    //==============================================================================
    // Content component for the dialog
    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent(ChordData& chordData, ChordModifierDialog& parent);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
    private:
        ChordData& modifiedChord;
        ChordModifierDialog& parentDialog;
        
        // UI sections
        std::unique_ptr<juce::GroupComponent> extensionGroup;
        std::unique_ptr<juce::GroupComponent> inversionGroup;
        std::unique_ptr<juce::GroupComponent> voicingGroup;
        std::unique_ptr<juce::GroupComponent> arpeggiatorGroup;
        std::unique_ptr<juce::GroupComponent> customVoicingGroup;
        
        // Extension controls
        std::unique_ptr<juce::ComboBox> extensionCombo;
        
        // Inversion controls
        std::unique_ptr<juce::ComboBox> inversionCombo;
        
        // Voicing controls
        std::unique_ptr<juce::ComboBox> voicingCombo;
        
        // Arpeggiator controls
        std::unique_ptr<juce::ComboBox> arpModeCombo;
        std::unique_ptr<juce::ComboBox> arpLengthCombo;
        
        // Custom voicing controls
        std::unique_ptr<juce::Slider> noteCountSlider;
        std::unique_ptr<juce::Label> noteCountLabel;
        std::unique_ptr<juce::Slider> positionSlider;
        std::unique_ptr<juce::Label> positionLabel;
        std::unique_ptr<juce::Slider> spreadSlider;
        std::unique_ptr<juce::Label> spreadLabel;
        
        // Action buttons
        std::unique_ptr<juce::TextButton> previewButton;
        std::unique_ptr<juce::TextButton> okButton;
        std::unique_ptr<juce::TextButton> cancelButton;
        
        void setupExtensionSection();
        void setupInversionSection();
        void setupVoicingSection();
        void setupArpeggiatorSection();
        void setupCustomVoicingSection();
        void setupActionButtons();
        void updateCustomVoicingVisibility();
        
        // Friend class for access to UI controls
        friend class ChordModifierDialog;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
    };
    
    //==============================================================================
    ChordData modifiedChord;
    std::unique_ptr<ContentComponent> contentComponent;
    std::unique_ptr<ModernLookAndFeel> modernLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordModifierDialog)
};

} // namespace ChordFoundry