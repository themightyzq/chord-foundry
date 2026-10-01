#include "ChordModifierDialog.h"
#include "../MusicTheory/MusicTheoryEngine.h"

namespace ChordFoundry {

namespace {

// The engine treats "" and "None" alike; the model keeps "" for "no modifier".
juce::String storedName(const juce::String& shown)
{
    return shown == "None" ? juce::String() : shown;
}

juce::String shownName(const juce::String& stored)
{
    return stored.isEmpty() ? juce::String("None") : stored;
}

void fillCombo(juce::ComboBox& combo, const std::vector<juce::String>& names, const juce::String& current)
{
    int selectedId = 1;
    for (size_t i = 0; i < names.size(); ++i)
    {
        combo.addItem(names[i], static_cast<int>(i) + 1);
        if (names[i] == current)
            selectedId = static_cast<int>(i) + 1;
    }

    // A value the engine does not know would play as "None"; show that instead of a blank box.
    combo.setSelectedId(selectedId, juce::dontSendNotification);
}

} // namespace

//==============================================================================
class ChordModifierDialog::Content : public juce::Component
{
public:
    Content(ChordModifierDialog& ownerDialog, const ChordData& chord)
        : owner(ownerDialog), edited(chord)
    {
        for (auto* group : { &extensionGroup, &inversionGroup, &voicingGroup, &arpGroup, &customGroup })
            addAndMakeVisible(*group);

        setupCombo(extensionCombo, "Chord extension", "Adds or replaces a note: +6th, +7th, +9th, sus2 or sus4",
                   MusicTheoryEngine::EXTENSION_NAMES, shownName(edited.extension),
                   [this] { edited.extension = storedName(extensionCombo.getText()); });

        setupCombo(inversionCombo, "Chord inversion", "Which note of the chord is lowest",
                   MusicTheoryEngine::INVERSION_NAMES, shownName(edited.inversion),
                   [this] { edited.inversion = storedName(inversionCombo.getText()); });

        setupCombo(voicingCombo, "Chord voicing", "How the notes are spread: root, open, drop 2 or custom",
                   MusicTheoryEngine::VOICING_NAMES, shownName(edited.voicing),
                   [this]
                   {
                       edited.voicing = storedName(voicingCombo.getText());
                       updateCustomVisibility();
                   });

        setupCombo(arpModeCombo, "Arpeggiator mode", "Play the chord as a sequence of single notes in this order (None plays it as a chord)",
                   MusicTheoryEngine::ARP_MODE_NAMES, shownName(edited.arpMode),
                   [this]
                   {
                       edited.arpMode = storedName(arpModeCombo.getText());
                       arpLengthCombo.setEnabled(edited.hasArpeggiator());
                   });

        setupCombo(arpLengthCombo, "Arpeggiator note length", "Length of each arpeggio note, in note values at the set tempo",
                   MusicTheoryEngine::ARP_LENGTH_NAMES, edited.arpLength.isEmpty() ? juce::String("1/8") : edited.arpLength,
                   [this] { edited.arpLength = arpLengthCombo.getText(); });
        arpLengthCombo.setEnabled(edited.hasArpeggiator());

        setupSlider(noteCountSlider, noteCountLabel, "Notes", "Number of notes in the custom voicing", 3, 8, edited.customVoicing.numNotes,
                    [this] { edited.customVoicing.numNotes = static_cast<int>(noteCountSlider.getValue()); });
        setupSlider(positionSlider, positionLabel, "Octave", "Octave position of the custom voicing", 0, 7, edited.customVoicing.position,
                    [this] { edited.customVoicing.position = static_cast<int>(positionSlider.getValue()); });

        for (size_t i = 0; i < MusicTheoryEngine::SPREAD_TYPE_NAMES.size(); ++i)
            spreadCombo.addItem(MusicTheoryEngine::SPREAD_TYPE_NAMES[i], static_cast<int>(i) + 1);
        spreadCombo.setSelectedId(juce::jlimit(0, static_cast<int>(MusicTheoryEngine::SPREAD_TYPE_NAMES.size()) - 1,
                                               edited.customVoicing.spreadType) + 1, juce::dontSendNotification);
        spreadCombo.onChange = [this] { edited.customVoicing.spreadType = spreadCombo.getSelectedId() - 1; };
        spreadCombo.setTitle("Custom voicing spread");
        spreadCombo.setTooltip("How the custom voicing spreads its notes");
        addAndMakeVisible(spreadCombo);
        spreadLabel.setText("Spread", juce::dontSendNotification);
        spreadLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
        addAndMakeVisible(spreadLabel);

        setupButton(previewButton, "Preview", "Play the chord with these settings", [this]
        {
            if (owner.onPreview)
                owner.onPreview(edited);
        });
        setupButton(cancelButton, "Cancel", "Close without changing the chord (Escape)", [this] { owner.finish(false); });
        setupButton(applyButton, "Apply", "Put these settings on the chord", [this] { owner.finish(true); });

        updateCustomVisibility();
    }

    ChordData getEdited() const { return edited; }

    int getPreferredHeight() const
    {
        return 16 + titleHeight + 4 * (groupHeight + gap) + (edited.voicing == "Custom" ? customHeight + gap : 0) + 44 + 16;
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ModernLookAndFeel::Colors::surface);
        g.setColour(ModernLookAndFeel::Colors::textPrimary);
        g.setFont(ModernLookAndFeel::Typography::getSubheaderFont());
        g.drawText("Modifiers for " + edited.roman, getLocalBounds().removeFromTop(16 + titleHeight).withTrimmedTop(8),
                   juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(titleHeight);

        const auto place = [&bounds](juce::GroupComponent& group, int height)
        {
            auto area = bounds.removeFromTop(height);
            group.setBounds(area);
            bounds.removeFromTop(gap);
            return area.reduced(14, 0).withTrimmedTop(22).withTrimmedBottom(8);
        };

        extensionCombo.setBounds(place(extensionGroup, groupHeight).withHeight(28));
        inversionCombo.setBounds(place(inversionGroup, groupHeight).withHeight(28));
        voicingCombo.setBounds(place(voicingGroup, groupHeight).withHeight(28));

        auto arp = place(arpGroup, groupHeight).withHeight(28);
        arpModeCombo.setBounds(arp.removeFromLeft((arp.getWidth() - 10) / 2));
        arp.removeFromLeft(10);
        arpLengthCombo.setBounds(arp);

        if (customGroup.isVisible())
        {
            auto area = place(customGroup, customHeight);
            const int labelWidth = 60;
            auto row = area.removeFromTop(28);
            noteCountLabel.setBounds(row.removeFromLeft(labelWidth));
            noteCountSlider.setBounds(row);
            area.removeFromTop(4);
            row = area.removeFromTop(28);
            positionLabel.setBounds(row.removeFromLeft(labelWidth));
            positionSlider.setBounds(row);
            area.removeFromTop(4);
            row = area.removeFromTop(28);
            spreadLabel.setBounds(row.removeFromLeft(labelWidth));
            spreadCombo.setBounds(row);
        }

        auto buttons = getLocalBounds().reduced(16).removeFromBottom(44);
        const int w = (buttons.getWidth() - 20) / 3;
        previewButton.setBounds(buttons.removeFromLeft(w));
        buttons.removeFromLeft(10);
        cancelButton.setBounds(buttons.removeFromLeft(w));
        buttons.removeFromLeft(10);
        applyButton.setBounds(buttons);
    }

private:
    static constexpr int titleHeight = 44;
    static constexpr int groupHeight = 64;
    static constexpr int customHeight = 140;
    static constexpr int gap = 10;

    void setupCombo(juce::ComboBox& combo, const juce::String& title, const juce::String& tip,
                    const std::vector<juce::String>& names, const juce::String& current,
                    std::function<void()> changed)
    {
        fillCombo(combo, names, current);
        combo.setTitle(title);
        combo.setTooltip(tip);
        combo.onChange = std::move(changed);
        addAndMakeVisible(combo);
    }

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name, const juce::String& tip,
                     int low, int high, int value, std::function<void()> changed)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 24);
        slider.setRange(low, high, 1);
        slider.setValue(juce::jlimit(low, high, value), juce::dontSendNotification);
        slider.onValueChange = std::move(changed);
        slider.setTitle(name);
        slider.setTooltip(tip);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
        addAndMakeVisible(label);
    }

    void setupButton(juce::TextButton& button, const juce::String& text, const juce::String& tip, std::function<void()> clicked)
    {
        button.setButtonText(text);
        button.setTooltip(tip);
        button.setTitle(text);
        button.onClick = std::move(clicked);
        addAndMakeVisible(button);
    }

    void updateCustomVisibility()
    {
        const bool custom = edited.voicing == "Custom";
        for (juce::Component* c : { static_cast<juce::Component*>(&customGroup), static_cast<juce::Component*>(&noteCountSlider),
                                    static_cast<juce::Component*>(&noteCountLabel), static_cast<juce::Component*>(&positionSlider),
                                    static_cast<juce::Component*>(&positionLabel), static_cast<juce::Component*>(&spreadCombo),
                                    static_cast<juce::Component*>(&spreadLabel) })
            c->setVisible(custom);

        if (owner.getContentComponent() == this)
            owner.setContentComponentSize(getWidth(), getPreferredHeight());
        resized();
    }

    ChordModifierDialog& owner;
    ChordData edited;

    juce::GroupComponent extensionGroup { "extension", "Extension" };
    juce::GroupComponent inversionGroup { "inversion", "Inversion" };
    juce::GroupComponent voicingGroup { "voicing", "Voicing" };
    juce::GroupComponent arpGroup { "arp", "Arpeggiator: mode and note length" };
    juce::GroupComponent customGroup { "custom", "Custom voicing" };

    juce::ComboBox extensionCombo, inversionCombo, voicingCombo, arpModeCombo, arpLengthCombo, spreadCombo;
    juce::Slider noteCountSlider, positionSlider;
    juce::Label noteCountLabel, positionLabel, spreadLabel;
    juce::TextButton previewButton, cancelButton, applyButton;
};

//==============================================================================
ChordModifierDialog::ChordModifierDialog(const ChordData& chord)
    : DialogWindow("Chord modifiers: " + chord.roman, ModernLookAndFeel::Colors::surface, true)
{
    setLookAndFeel(&lookAndFeel);

    content = new Content(*this, chord);
    content->setSize(480, content->getPreferredHeight());
    setContentOwned(content, true);

    setResizable(false, false);
    setUsingNativeTitleBar(true);
}

ChordModifierDialog::~ChordModifierDialog()
{
    setLookAndFeel(nullptr);
}

ChordData ChordModifierDialog::getEditedChord() const
{
    return content != nullptr ? content->getEdited() : ChordData();
}

void ChordModifierDialog::launch(juce::Component* centreAround)
{
    centreAroundComponent(centreAround, getWidth(), getHeight());
    setVisible(true);
    enterModalState(true, nullptr, true);   // deleted when dismissed
}

void ChordModifierDialog::closeButtonPressed()
{
    finish(false);
}

void ChordModifierDialog::finish(bool apply)
{
    if (finished)
        return;

    finished = true;

    if (apply && onApply)
        onApply(getEditedChord());

    if (onClosed)
        onClosed();

    exitModalState(apply ? 1 : 0);
}

} // namespace ChordFoundry
