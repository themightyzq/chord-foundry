#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <random>
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

    // Same, but "Random" is shuffled from `seed`, so the same notes and seed always give the
    // same order. Playback and MIDI export use this with seedFor(), so a block's random order
    // does not change when something else in the project is edited, and the exported file
    // matches what was heard.
    std::vector<float> getArpeggioSequence(const std::vector<float>& notes, const juce::String& mode,
                                           std::uint32_t seed);

    // A seed derived from a block's content: its MIDI notes and first step. Editing other
    // blocks, the tempo, or the block's length does not change it.
    static std::uint32_t seedFor(const std::vector<int>& midiNotes, int startStep);

    // Returns the note length in seconds for a given arp-length division
    // ("1/16", "1/8", "1/4", "1/2") at the given tempo (BPM). Unrecognised
    // divisions default to "1/16". tempo is beats (quarter notes) per minute.
    double getNoteLength(const juce::String& arpLength, float tempo);

    // Length of one arp note in quarter-note beats (1/16 = 0.25 ... 1/2 = 2.0);
    // unrecognised divisions default to 1/16.
    static double getNoteLengthBeats(const juce::String& arpLength);

    // The same length in sequencer steps (16th notes): 1, 2, 4 or 8. The step
    // scheduler uses this so arp notes land exactly on step boundaries.
    static int getNoteLengthSteps(const juce::String& arpLength);

private:
    static std::vector<float> reorder(const std::vector<float>& notes, const juce::String& mode,
                                      std::mt19937& rng);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorEngine)
};

} // namespace ChordFoundry
