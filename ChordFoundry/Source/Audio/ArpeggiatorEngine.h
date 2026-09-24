#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace ChordFoundry {

// Pure list-manipulation arpeggiator: reorders a chord's notes into a
// playback sequence and maps an arp-length division to a note duration.
// Ported from archive/core/arpeggiator.py (get_arpeggio_sequence /
// get_note_duration), the working Python original this app was rewritten
// from. No audio or JUCE audio-module dependency.
class ArpeggiatorEngine {
public:
    ArpeggiatorEngine() = default;
    ~ArpeggiatorEngine() = default;

    // Reorders `notes` according to `mode`.
    // Supported modes: "Up", "Down", "Random", "Converge", "Diverge",
    // "Ascending", "Descending". An empty/"None"/unrecognised mode, or an
    // empty `notes` vector, returns `notes` unchanged.
    std::vector<float> getArpeggioSequence(const std::vector<float>& notes, const juce::String& mode);

    // Returns the note length in seconds for a given arp-length division
    // ("1/16", "1/8", "1/4", "1/2") at the given tempo (BPM). Unrecognised
    // divisions default to "1/16". tempo is beats (quarter notes) per minute.
    double getNoteLength(const juce::String& arpLength, float tempo);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorEngine)
};

} // namespace ChordFoundry
