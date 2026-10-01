#include "SequencerPatternBuilder.h"

#include "ArpeggiatorEngine.h"
#include "../MusicTheory/MusicTheoryEngine.h"

namespace ChordFoundry {

SequencerPattern SequencerPatternBuilder::build (const std::vector<ChordData>& chords,
                                                 const std::vector<BlockData>& blocks,
                                                 const juce::String& key,
                                                 const juce::String& mode)
{
    SequencerPattern pattern;
    pattern.clear();

    ArpeggiatorEngine arpeggiator;

    for (const auto& block : blocks)
    {
        if (block.chordIndex < 0 || block.chordIndex >= static_cast<int> (chords.size()))
            continue;

        const auto effective = block.getEffectiveChordData (chords[static_cast<size_t> (block.chordIndex)]);
        const auto midiNotes = MusicTheoryEngine::getChordMidiNotes (effective.roman, key, mode, effective);

        SequencerSlot slot;
        slot.velocity = defaultVelocity;

        if (effective.hasArpeggiator())
        {
            std::vector<float> asFloat;
            asFloat.reserve (midiNotes.size());
            for (const auto note : midiNotes)
                asFloat.push_back (static_cast<float> (note));

            // Random is shuffled from the block's own content so unrelated edits keep its order.
            const auto ordered = arpeggiator.getArpeggioSequence (asFloat, effective.arpMode,
                                                                  ArpeggiatorEngine::seedFor (midiNotes, block.startStep));
            for (const auto note : ordered)
            {
                if (slot.numNotes < SequencerSlot::maxNotes)
                    slot.notes[slot.numNotes++] = static_cast<std::uint8_t> (juce::jlimit (0, 127, juce::roundToInt (note)));
            }

            slot.arpSteps = ArpeggiatorEngine::getNoteLengthSteps (effective.arpLength);
        }
        else
        {
            for (const auto note : midiNotes)
            {
                if (slot.numNotes < SequencerSlot::maxNotes)
                    slot.notes[slot.numNotes++] = static_cast<std::uint8_t> (juce::jlimit (0, 127, note));
            }
        }

        if (slot.numNotes == 0)
            continue;

        for (int step = block.startStep; step < block.startStep + block.lengthSteps; ++step)
        {
            if (step < 0 || step >= SequencerPattern::numSteps)
                continue;

            auto& target = pattern.steps[step];
            if (target.numSlots >= SequencerPattern::maxSlots)
                continue;

            auto copy = slot;
            copy.strike = block.newStrike && step == block.startStep;
            target.slots[target.numSlots++] = copy;
        }
    }

    return pattern;
}

} // namespace ChordFoundry
