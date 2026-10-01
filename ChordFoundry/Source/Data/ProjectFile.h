#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ChordProgression.h"

namespace ChordFoundry {

// The session settings that are saved with a project next to the chord progression
// (chords, per-chord modifiers and arpeggiator settings, and the step-sequencer blocks).
struct ProjectSettings {
    float tempo = 120.0f;                  // BPM, 40-240
    juce::String key = "C";
    juce::String mode = "Major";
    bool loop = false;
    bool clickTrack = false;
    float masterVolume = 0.7f;             // synth output level, 0-1

    bool operator==(const ProjectSettings& other) const {
        return tempo == other.tempo && key == other.key && mode == other.mode &&
               loop == other.loop && clickTrack == other.clickTrack &&
               masterVolume == other.masterVolume;
    }
    bool operator!=(const ProjectSettings& other) const { return !(*this == other); }
};

// Reads and writes Chord Foundry project files.
//
// Format: XML. The root element is <ChordFoundryProject formatVersion="N">, holding a
// <Settings> element and the <ChordProgression> tree. formatVersion is bumped when the
// layout changes in a way older builds cannot read; a file with a higher version than this
// build knows is refused with a clear message instead of being half-loaded.
//
// Saving writes a temporary file beside the target and swaps it in, so an existing project
// is never left truncated or half-written if a save fails. Loading fills the outputs only
// when the whole file is valid.
class ProjectFile {
public:
    static constexpr int currentFormatVersion = 1;
    static const char* const fileExtension;      // ".cfproj"
    static const char* const fileWildcard;       // "*.cfproj"

    // The text that save() would write. Deterministic: the same state always yields the
    // same string, which is how the app tells whether a project has unsaved changes.
    static juce::String toXmlString(const ProjectSettings& settings, const ChordProgression& progression);

    static juce::Result save(const juce::File& file,
                             const ProjectSettings& settings,
                             const ChordProgression& progression);

    // On success settings and progression hold the loaded project. On failure they are
    // untouched and the Result carries a message for the user.
    static juce::Result load(const juce::File& file,
                             ProjectSettings& settings,
                             ChordProgression& progression);

    static juce::Result parse(const juce::String& xmlText,
                              ProjectSettings& settings,
                              ChordProgression& progression);
};

} // namespace ChordFoundry
