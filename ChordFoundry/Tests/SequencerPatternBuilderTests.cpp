#include "../Source/Audio/SequencerPatternBuilder.h"
#include "../Source/Audio/StepSequencer.h"
#include "../Source/Audio/ChordSynthesizer.h"
#include "../Source/MusicTheory/MusicTheoryEngine.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

using namespace ChordFoundry;

namespace {

BlockData makeBlock(int chordIndex, int start, int length)
{
    return BlockData(chordIndex, start, length, juce::Colours::grey);
}

} // namespace

// Model -> flat pattern -> step scheduler, the path MainComponent uses for playback.
class SequencerPatternBuilderTests : public juce::UnitTest
{
public:
    SequencerPatternBuilderTests() : juce::UnitTest("SequencerPatternBuilder", "Audio") {}

    void runTest() override
    {
        const std::vector<ChordData> chords { ChordData("I"), ChordData("V") };

        beginTest("a block fills every step it covers with the chord's notes");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(0, 2, 3) }, "C", "Major");

            for (int s = 0; s < SequencerPattern::numSteps; ++s)
                expectEquals(pattern.steps[s].numSlots, (s >= 2 && s < 5) ? 1 : 0, "step " + juce::String(s));

            const auto& slot = pattern.steps[2].slots[0];
            expect(slot.numNotes >= 3, "a triad has at least three notes");
            expectEquals(slot.arpSteps, 0, "no arpeggiator");
            expect(! slot.strike, "no forced strike");
            expect(slot.sameVoicing(pattern.steps[3].slots[0]), "the same chord on every step");
            expect(slot.sameVoicing(pattern.steps[4].slots[0]));
        }

        beginTest("adjacent blocks of the same chord produce identical slots (so playback sustains)");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(0, 0, 2), makeBlock(0, 2, 2) },
                                                                "C", "Major");
            expect(pattern.steps[1].slots[0].sameVoicing(pattern.steps[2].slots[0]));
        }

        beginTest("different chords produce different slots");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(0, 0, 2), makeBlock(1, 2, 2) },
                                                                "C", "Major");
            expect(! pattern.steps[1].slots[0].sameVoicing(pattern.steps[2].slots[0]));
        }

        beginTest("newStrike marks only the first step of its block");
        {
            auto block = makeBlock(0, 4, 3);
            block.newStrike = true;
            const auto pattern = SequencerPatternBuilder::build(chords, { block }, "C", "Major");

            expect(pattern.steps[4].slots[0].strike);
            expect(! pattern.steps[5].slots[0].strike);
            expect(! pattern.steps[6].slots[0].strike);
        }

        beginTest("an arpeggiated chord carries its note order and rate");
        {
            ChordData arp("I");
            arp.arpMode = "Down";
            arp.arpLength = "1/8";
            const auto plain = SequencerPatternBuilder::build({ ChordData("I") }, { makeBlock(0, 0, 1) }, "C", "Major");
            const auto pattern = SequencerPatternBuilder::build({ arp }, { makeBlock(0, 0, 4) }, "C", "Major");

            const auto& slot = pattern.steps[0].slots[0];
            expectEquals(slot.arpSteps, 2, "1/8 is two 16th-note steps");
            expectEquals(slot.numNotes, plain.steps[0].slots[0].numNotes);
            expectEquals(static_cast<int>(slot.notes[0]),
                         static_cast<int>(plain.steps[0].slots[0].notes[plain.steps[0].slots[0].numNotes - 1]),
                         "Down starts from the top note");
        }

        beginTest("per-block overrides replace the base chord's modifiers");
        {
            auto block = makeBlock(0, 0, 1);
            block.hasModifierOverrides = true;
            block.modifierOverrides.arpMode = "Up";
            block.modifierOverrides.arpLength = "1/4";
            const auto pattern = SequencerPatternBuilder::build(chords, { block }, "C", "Major");
            expectEquals(pattern.steps[0].slots[0].arpSteps, 4);
        }

        beginTest("overlapping blocks of different chords sound together");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(0, 0, 4), makeBlock(1, 2, 4) },
                                                                "C", "Major");
            expectEquals(pattern.steps[1].numSlots, 1);
            expectEquals(pattern.steps[2].numSlots, 2);
            expectEquals(pattern.steps[5].numSlots, 1);
        }

        beginTest("the key and mode labels the settings panel offers produce chords");
        {
            // "C#/Db" and "Major (Ionian)" used to match nothing, so those choices played silence.
            for (const char* key : { "C", "C#/Db", "D#/Eb", "F#/Gb", "G#/Ab", "A#/Bb" })
                for (const char* mode : { "Major (Ionian)", "Minor (Aeolian)", "Dorian", "Whole Tone" })
                {
                    const auto notes = MusicTheoryEngine::getChordMidiNotes("I", key, mode, ChordData("I"));
                    expect(notes.size() >= 3, juce::String("I in ") + key + " " + mode + " has notes");
                }

            const auto plain = MusicTheoryEngine::getChordMidiNotes("I", "C#", "Major", ChordData("I"));
            const auto label = MusicTheoryEngine::getChordMidiNotes("I", "C#/Db", "Major (Ionian)", ChordData("I"));
            expect(plain == label, "a label means the same as the table name");
        }

        beginTest("the diminished chord is named vii followed by a degree sign and sounds");
        {
            const auto name = diminishedRoman();
            expectEquals(name.length(), 4, "vii + one degree sign character, not a mis-decoded pair");
            expect(MusicTheoryEngine::isValidRoman(name));
            expect(MusicTheoryEngine::getChordMidiNotes(name, "C", "Major", ChordData(name)).size() >= 3);
        }

        beginTest("a Random arpeggio keeps its order when other blocks are edited");
        {
            ChordData random("I");
            random.extension = "+7th";
            random.arpMode = "Random";
            random.arpLength = "1/16";
            const std::vector<ChordData> withRandom { random, ChordData("V"), ChordData("IV") };

            const auto base = SequencerPatternBuilder::build(withRandom, { makeBlock(0, 0, 8) }, "C", "Major");
            const auto& reference = base.steps[0].slots[0];
            expect(reference.arpSteps == 1 && reference.numNotes >= 4);

            // Unrelated edits: other blocks added, moved and removed, a longer block, repeated rebuilds.
            const std::vector<std::vector<BlockData>> edits {
                { makeBlock(0, 0, 8), makeBlock(1, 12, 4) },
                { makeBlock(0, 0, 8), makeBlock(1, 12, 4), makeBlock(2, 20, 6) },
                { makeBlock(0, 0, 8), makeBlock(2, 9, 2) },
                { makeBlock(0, 0, 12), makeBlock(1, 16, 8) },
            };

            for (int round = 0; round < 6; ++round)
                for (const auto& blocks : edits)
                {
                    const auto rebuilt = SequencerPatternBuilder::build(withRandom, blocks, "C", "Major");
                    expect(rebuilt.steps[0].slots[0].sameVoicing(reference), "same order after an unrelated edit");
                    expect(rebuilt.steps[7].slots[0].sameVoicing(reference), "same order on every step of the block");
                }
        }

        beginTest("replacing the pattern during playback does not re-strike a Random arpeggio");
        {
            ChordData random("I");
            random.extension = "+7th";
            random.arpMode = "Random";
            random.arpLength = "1/16";
            const std::vector<ChordData> chordsR { random, ChordData("V") };

            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);
            seq.setPattern(SequencerPatternBuilder::build(chordsR, { makeBlock(0, 0, 12) }, "C", "Major"));
            seq.startNow();

            std::vector<int> played;
            juce::int64 blockStart = 0;
            for (int block = 0; block < 12 * 6000 / 500; ++block)
            {
                if (blockStart >= 5 * 6000 && blockStart < 5 * 6000 + 500)   // an edit elsewhere, mid-block
                    seq.setPattern(SequencerPatternBuilder::build(chordsR, { makeBlock(0, 0, 12), makeBlock(1, 20, 4) }, "C", "Major"));

                int done = 0;
                while (done < 500)
                {
                    const int consumed = seq.process(500 - done);
                    for (int i = 0; i < seq.getNumEvents(); ++i)
                        if (seq.getEvents()[i].type == StepSequencer::Event::Type::noteOn)
                            played.push_back(seq.getEvents()[i].note);
                    done += consumed;
                }
                blockStart += 500;
            }

            // One note per step for 12 steps, following the same cyclic order throughout.
            expectEquals(static_cast<int>(played.size()), 12);
            bool cyclic = played.size() == 12;
            for (size_t i = 4; cyclic && i < played.size(); ++i)
                cyclic = played[i] == played[i - 4];
            expect(cyclic, "the arpeggio continued its cycle across the edit");
        }

        beginTest("blocks pointing at a missing chord are skipped");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(5, 0, 2), makeBlock(-1, 2, 2) },
                                                                "C", "Major");
            for (const auto& step : pattern.steps)
                expectEquals(step.numSlots, 0);
        }

        beginTest("end to end: four steps of one chord struck once; two chords struck twice");
        {
            const auto pattern = SequencerPatternBuilder::build(chords, { makeBlock(0, 0, 2), makeBlock(0, 2, 2) },
                                                                "C", "Major");
            const int notesPerChord = pattern.steps[0].slots[0].numNotes;

            ChordSynthAudioSource source;
            source.prepareToPlay(480, 48000.0);
            source.setTempo(120.0f);
            source.setLoop(false);
            source.setSequencerPattern(pattern);
            source.startSequencer();

            juce::AudioBuffer<float> out(2, 480);
            for (int i = 0; i < 4 * 6000 / 480; ++i)
            {
                juce::AudioSourceChannelInfo info(&out, 0, 480);
                source.getNextAudioBlock(info);
            }
            expectEquals(source.getNoteOnCount(), notesPerChord, "one strike for the whole four steps");

            const auto changing = SequencerPatternBuilder::build(chords, { makeBlock(0, 0, 2), makeBlock(1, 2, 2) },
                                                                 "C", "Major");
            ChordSynthAudioSource second;
            second.prepareToPlay(480, 48000.0);
            second.setTempo(120.0f);
            second.setLoop(false);
            second.setSequencerPattern(changing);
            second.startSequencer();
            for (int i = 0; i < 4 * 6000 / 480; ++i)
            {
                juce::AudioSourceChannelInfo info(&out, 0, 480);
                second.getNextAudioBlock(info);
            }
            expectEquals(second.getNoteOnCount(),
                         changing.steps[0].slots[0].numNotes + changing.steps[2].slots[0].numNotes,
                         "a new strike when the chord changes");
        }
    }
};

static SequencerPatternBuilderTests sequencerPatternBuilderTestsInstance;
