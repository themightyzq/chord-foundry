#include "ChordModifierDialog.h"

namespace ChordFoundry {

//==============================================================================
// ChordModifierDialog Implementation
ChordModifierDialog::ChordModifierDialog(const ChordData& chord)
    : DialogWindow("Edit Chord: " + chord.roman, ModernLookAndFeel::Colors::surface, true),
      modifiedChord(chord)
{
    // Set up modern look and feel
    modernLookAndFeel = std::make_unique<ModernLookAndFeel>();
    setLookAndFeel(modernLookAndFeel.get());
    
    // Create content component
    contentComponent = std::make_unique<ContentComponent>(modifiedChord, *this);
    setContentOwned(contentComponent.get(), true);
    
    // Configure dialog properties
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    setDropShadowEnabled(true);
    
    // Set dialog size
    setSize(500, 600);
    
    // Center on screen
    centreAroundComponent(nullptr, getWidth(), getHeight());
}

void ChordModifierDialog::buttonClicked(juce::Button* button)
{
    if (!contentComponent) return;
    
    if (button == contentComponent->previewButton.get())
    {
        // Preview the chord modifications
        if (onPreviewChord)
            onPreviewChord();
    }
    else if (button == contentComponent->okButton.get())
    {
        // Apply changes and close dialog
        updateChordFromUI();
        exitModalState(1); // OK result
    }
    else if (button == contentComponent->cancelButton.get())
    {
        // Cancel changes and close dialog
        exitModalState(0); // Cancel result
    }
}

void ChordModifierDialog::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (!contentComponent) return;
    
    if (comboBox == contentComponent->extensionCombo.get())
    {
        auto text = comboBox->getText();
        modifiedChord.extension = (text == "None") ? "" : text;
    }
    else if (comboBox == contentComponent->inversionCombo.get())
    {
        auto text = comboBox->getText();
        modifiedChord.inversion = (text == "Root Position") ? "" : text;
    }
    else if (comboBox == contentComponent->voicingCombo.get())
    {
        auto text = comboBox->getText();
        modifiedChord.voicing = text;
        contentComponent->updateCustomVoicingVisibility();
    }
    else if (comboBox == contentComponent->arpModeCombo.get())
    {
        auto text = comboBox->getText();
        modifiedChord.arpMode = (text == "None") ? "" : text;
    }
    else if (comboBox == contentComponent->arpLengthCombo.get())
    {
        modifiedChord.arpLength = comboBox->getText();
    }
}

void ChordModifierDialog::sliderValueChanged(juce::Slider* slider)
{
    if (!contentComponent) return;
    
    if (slider == contentComponent->noteCountSlider.get())
    {
        modifiedChord.customVoicing.numNotes = static_cast<int>(slider->getValue());
    }
    else if (slider == contentComponent->positionSlider.get())
    {
        modifiedChord.customVoicing.position = static_cast<int>(slider->getValue());
    }
    else if (slider == contentComponent->spreadSlider.get())
    {
        modifiedChord.customVoicing.spreadType = static_cast<int>(slider->getValue());
    }
}

void ChordModifierDialog::updateChordFromUI()
{
    // All updates are done in real-time via the listeners above
    // This method is here for any final validation or processing if needed
}

//==============================================================================
// ContentComponent Implementation
ChordModifierDialog::ContentComponent::ContentComponent(ChordData& chordData, ChordModifierDialog& parent)
    : modifiedChord(chordData), parentDialog(parent)
{
    setupExtensionSection();
    setupInversionSection();
    setupVoicingSection();
    setupArpeggiatorSection();
    setupCustomVoicingSection();
    setupActionButtons();
    
    updateCustomVoicingVisibility();
}

void ChordModifierDialog::ContentComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(ModernLookAndFeel::Colors::background);
    
    // Draw header with chord name
    auto headerBounds = getLocalBounds().removeFromTop(50);
    
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(ModernLookAndFeel::Typography::getHeaderFont());
    g.drawText("Editing: " + modifiedChord.roman, headerBounds, juce::Justification::centred);
}

void ChordModifierDialog::ContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(ModernLookAndFeel::Metrics::spacingLG);
    
    // Header space
    bounds.removeFromTop(50);
    
    // Extension section
    if (extensionGroup)
    {
        extensionGroup->setBounds(bounds.removeFromTop(80));
        bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    // Inversion section
    if (inversionGroup)
    {
        inversionGroup->setBounds(bounds.removeFromTop(80));
        bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    // Voicing section
    if (voicingGroup)
    {
        voicingGroup->setBounds(bounds.removeFromTop(80));
        bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    // Arpeggiator section
    if (arpeggiatorGroup)
    {
        arpeggiatorGroup->setBounds(bounds.removeFromTop(100));
        bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    // Custom voicing section (variable height based on visibility)
    if (customVoicingGroup && customVoicingGroup->isVisible())
    {
        customVoicingGroup->setBounds(bounds.removeFromTop(160));
        bounds.removeFromTop(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    // Action buttons at bottom
    auto buttonArea = bounds.removeFromBottom(ModernLookAndFeel::Metrics::buttonHeight);
    auto buttonWidth = (buttonArea.getWidth() - (ModernLookAndFeel::Metrics::spacingMD * 2)) / 3;
    
    if (previewButton)
    {
        previewButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    if (cancelButton)
    {
        cancelButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(ModernLookAndFeel::Metrics::spacingMD);
    }
    
    if (okButton)
        okButton->setBounds(buttonArea);
}

void ChordModifierDialog::ContentComponent::setupExtensionSection()
{
    extensionGroup = std::make_unique<juce::GroupComponent>("extensionGroup", "Extensions");
    extensionGroup->setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(*extensionGroup);
    
    extensionCombo = std::make_unique<juce::ComboBox>("extensionCombo");
    extensionCombo->addItemList({
        "None", "7", "maj7", "9", "maj9", "11", "maj11", "13", "maj13",
        "add9", "add11", "sus2", "sus4", "6", "dim", "aug", "b5", "#5"
    }, 1);
    
    // Set current value or default
    auto currentExtension = modifiedChord.extension.isEmpty() ? "None" : modifiedChord.extension;
    extensionCombo->setText(currentExtension, juce::dontSendNotification);
    
    extensionCombo->addListener(&parentDialog);
    extensionCombo->setAccessible(true);
    extensionCombo->setTitle("Chord extension");
    extensionGroup->addAndMakeVisible(*extensionCombo);
    
    // Position within group
    extensionCombo->setBounds(20, 30, extensionGroup->getWidth() - 40, 30);
}

void ChordModifierDialog::ContentComponent::setupInversionSection()
{
    inversionGroup = std::make_unique<juce::GroupComponent>("inversionGroup", "Inversions");
    inversionGroup->setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(*inversionGroup);
    
    inversionCombo = std::make_unique<juce::ComboBox>("inversionCombo");
    inversionCombo->addItemList({"Root Position", "1st Inversion", "2nd Inversion", "3rd Inversion"}, 1);
    
    // Set current value or default
    auto currentInversion = modifiedChord.inversion.isEmpty() ? "Root Position" : modifiedChord.inversion;
    inversionCombo->setText(currentInversion, juce::dontSendNotification);
    
    inversionCombo->addListener(&parentDialog);
    inversionCombo->setAccessible(true);
    inversionCombo->setTitle("Chord inversion");
    inversionGroup->addAndMakeVisible(*inversionCombo);
    
    // Position within group
    inversionCombo->setBounds(20, 30, inversionGroup->getWidth() - 40, 30);
}

void ChordModifierDialog::ContentComponent::setupVoicingSection()
{
    voicingGroup = std::make_unique<juce::GroupComponent>("voicingGroup", "Voicing");
    voicingGroup->setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(*voicingGroup);
    
    voicingCombo = std::make_unique<juce::ComboBox>("voicingCombo");
    voicingCombo->addItemList({"Close", "Open", "Drop 2", "Drop 3", "Spread", "Custom"}, 1);
    
    // Set current value or default
    auto currentVoicing = modifiedChord.voicing.isEmpty() ? "Close" : modifiedChord.voicing;
    voicingCombo->setText(currentVoicing, juce::dontSendNotification);
    
    voicingCombo->addListener(&parentDialog);
    voicingCombo->setAccessible(true);
    voicingCombo->setTitle("Chord voicing style");
    voicingGroup->addAndMakeVisible(*voicingCombo);
    
    // Position within group
    voicingCombo->setBounds(20, 30, voicingGroup->getWidth() - 40, 30);
}

void ChordModifierDialog::ContentComponent::setupArpeggiatorSection()
{
    arpeggiatorGroup = std::make_unique<juce::GroupComponent>("arpeggiatorGroup", "Arpeggiator");
    arpeggiatorGroup->setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(*arpeggiatorGroup);
    
    // Arpeggiator mode
    arpModeCombo = std::make_unique<juce::ComboBox>("arpModeCombo");
    arpModeCombo->addItemList({"None", "Up", "Down", "Up-Down", "Down-Up", "Random", "Chord"}, 1);
    
    auto currentArpMode = modifiedChord.arpMode.isEmpty() ? "None" : modifiedChord.arpMode;
    arpModeCombo->setText(currentArpMode, juce::dontSendNotification);
    
    arpModeCombo->addListener(&parentDialog);
    arpModeCombo->setAccessible(true);
    arpModeCombo->setTitle("Arpeggiator mode");
    arpeggiatorGroup->addAndMakeVisible(*arpModeCombo);
    
    // Arpeggiator length
    arpLengthCombo = std::make_unique<juce::ComboBox>("arpLengthCombo");
    arpLengthCombo->addItemList({"1/16", "1/8", "1/4", "1/2", "1/1", "2/1"}, 1);
    
    auto currentArpLength = modifiedChord.arpLength.isEmpty() ? "1/8" : modifiedChord.arpLength;
    arpLengthCombo->setText(currentArpLength, juce::dontSendNotification);
    
    arpLengthCombo->addListener(&parentDialog);
    arpLengthCombo->setAccessible(true);
    arpLengthCombo->setTitle("Arpeggiator note length");
    arpeggiatorGroup->addAndMakeVisible(*arpLengthCombo);
    
    // Position within group
    arpModeCombo->setBounds(20, 30, (arpeggiatorGroup->getWidth() - 60) / 2, 30);
    arpLengthCombo->setBounds(30 + (arpeggiatorGroup->getWidth() - 60) / 2, 30, (arpeggiatorGroup->getWidth() - 60) / 2, 30);
}

void ChordModifierDialog::ContentComponent::setupCustomVoicingSection()
{
    customVoicingGroup = std::make_unique<juce::GroupComponent>("customVoicingGroup", "Custom Voicing");
    customVoicingGroup->setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(*customVoicingGroup);
    
    // Note count slider
    noteCountSlider = std::make_unique<juce::Slider>("noteCountSlider");
    noteCountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    noteCountSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    noteCountSlider->setRange(3, 8, 1);
    noteCountSlider->setValue(modifiedChord.customVoicing.numNotes);
    noteCountSlider->addListener(&parentDialog);
    customVoicingGroup->addAndMakeVisible(*noteCountSlider);
    
    noteCountLabel = std::make_unique<juce::Label>("noteCountLabel", "Notes:");
    noteCountLabel->attachToComponent(noteCountSlider.get(), true);
    customVoicingGroup->addAndMakeVisible(*noteCountLabel);
    
    // Position slider
    positionSlider = std::make_unique<juce::Slider>("positionSlider");
    positionSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    positionSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    positionSlider->setRange(0, 7, 1);
    positionSlider->setValue(modifiedChord.customVoicing.position);
    positionSlider->addListener(&parentDialog);
    customVoicingGroup->addAndMakeVisible(*positionSlider);
    
    positionLabel = std::make_unique<juce::Label>("positionLabel", "Octave:");
    positionLabel->attachToComponent(positionSlider.get(), true);
    customVoicingGroup->addAndMakeVisible(*positionLabel);
    
    // Spread slider
    spreadSlider = std::make_unique<juce::Slider>("spreadSlider");
    spreadSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    spreadSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    spreadSlider->setRange(0, 5, 1);
    spreadSlider->setValue(modifiedChord.customVoicing.spreadType);
    spreadSlider->addListener(&parentDialog);
    customVoicingGroup->addAndMakeVisible(*spreadSlider);
    
    spreadLabel = std::make_unique<juce::Label>("spreadLabel", "Spread:");
    spreadLabel->attachToComponent(spreadSlider.get(), true);
    customVoicingGroup->addAndMakeVisible(*spreadLabel);
    
    // Position sliders within group
    noteCountSlider->setBounds(80, 30, customVoicingGroup->getWidth() - 100, 25);
    positionSlider->setBounds(80, 60, customVoicingGroup->getWidth() - 100, 25);
    spreadSlider->setBounds(80, 90, customVoicingGroup->getWidth() - 100, 25);
}

void ChordModifierDialog::ContentComponent::setupActionButtons()
{
    // Preview button
    previewButton = std::make_unique<juce::TextButton>("Preview");
    previewButton->addListener(&parentDialog);
    previewButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::info);
    previewButton->setAccessible(true);
    previewButton->setTitle("Preview chord modifications");
    addAndMakeVisible(*previewButton);
    
    // Cancel button
    cancelButton = std::make_unique<juce::TextButton>("Cancel");
    cancelButton->addListener(&parentDialog);
    cancelButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::textSecondary);
    cancelButton->setAccessible(true);
    cancelButton->setTitle("Cancel changes");
    addAndMakeVisible(*cancelButton);
    
    // OK button
    okButton = std::make_unique<juce::TextButton>("Apply");
    okButton->addListener(&parentDialog);
    okButton->setColour(juce::TextButton::buttonOnColourId, ModernLookAndFeel::Colors::primary);
    okButton->setAccessible(true);
    okButton->setTitle("Apply chord modifications");
    addAndMakeVisible(*okButton);
}

void ChordModifierDialog::ContentComponent::updateCustomVoicingVisibility()
{
    bool showCustom = voicingCombo && voicingCombo->getText() == "Custom";
    if (customVoicingGroup)
        customVoicingGroup->setVisible(showCustom);
    
    // Resize dialog if needed
    parentDialog.setSize(500, showCustom ? 760 : 600);
    resized();
}

} // namespace ChordFoundry
