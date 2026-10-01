#pragma once

#include "StepSequencer.h"
#include "../Data/ChordProgression.h"

namespace ChordFoundry {

// Turns the editable model (chords + pattern blocks, key and mode) into the flat
// SequencerPattern the audio thread plays. Runs on the message thread.
//
// Every block contributes one slot to each step it covers. The slot carries the block's
// effective chord (the base chord plus any per-block overrides) as MIDI notes, put in
// arpeggio order when the chord has an arpeggiator. A slot is flagged as a new strike only
// at the first step of a block whose newStrike flag is set; otherwise the sequencer decides
// by comparing neighbouring steps, so identical chords on consecutive steps sustain.
class SequencerPatternBuilder
{
public:
    static SequencerPattern build (const std::vector<ChordData>& chords,
                                   const std::vector<BlockData>& blocks,
                                   const juce::String& key,
                                   const juce::String& mode);

    static SequencerPattern build (const ChordProgression& progression,
                                   const juce::String& key,
                                   const juce::String& mode)
    {
        return build (progression.getChords(), progression.getBlocks(), key, mode);
    }

    static constexpr float defaultVelocity = 0.8f;
};

} // namespace ChordFoundry
