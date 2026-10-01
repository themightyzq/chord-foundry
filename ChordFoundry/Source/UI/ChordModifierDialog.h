#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../Data/ChordProgression.h"
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * ChordModifierDialog - dialog for editing one chord's modifiers.
 *
 * Offers exactly the values the music-theory engine and arpeggiator understand
 * (MusicTheoryEngine::EXTENSION_NAMES, INVERSION_NAMES, VOICING_NAMES, ARP_MODE_NAMES,
 * ARP_LENGTH_NAMES, SPREAD_TYPE_NAMES), so every choice changes the sound:
 * - Extension (+6th, +7th, +9th, sus2, sus4)
 * - Inversion (root, 1st, 2nd)
 * - Voicing (root, open, drop 2, custom)
 * - Arpeggiator (mode and note length)
 * - Custom voicing (note count, octave, spread type), shown when the voicing is Custom
 *
 * It does not change the chord itself: Apply hands the edited copy to onApply and the
 * owner puts it into the progression. Cancel, Escape and the close button discard.
 * Show it with launch(); it deletes itself when dismissed.
 */
class ChordModifierDialog : public juce::DialogWindow
{
public:
    explicit ChordModifierDialog(const ChordData& chord);
    ~ChordModifierDialog() override;

    // Called with the edited chord when Apply is pressed, before the dialog closes.
    std::function<void(const ChordData&)> onApply;

    // Called with the chord as currently edited when Preview is pressed.
    std::function<void(const ChordData&)> onPreview;

    // Called once when the dialog closes, whether applied or cancelled.
    std::function<void()> onClosed;

    // Shows the dialog modally and positions it over `centreAround` (may be null).
    void launch(juce::Component* centreAround);

    void closeButtonPressed() override;

    ChordData getEditedChord() const;

private:
    class Content;
    void finish(bool apply);

    ModernLookAndFeel lookAndFeel;
    Content* content = nullptr;   // owned by the window
    bool finished = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordModifierDialog)
};

} // namespace ChordFoundry
