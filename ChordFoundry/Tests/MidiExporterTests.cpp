#include "../Source/Export/MidiExporter.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

using namespace ChordFoundry;

namespace {

BlockData makeBlock(int chordIndex, int start, int length)
{
    return BlockData(chordIndex, start, length, juce::Colours::grey);
}

// Reads `file` back as a MIDI file. Returns false if it does not parse.
bool readBack(const juce::File& file, juce::MidiFile& midi)
{
    juce::FileInputStream in(file);
    return in.openedOk() && midi.readFrom(in);
}

int countEvents(const juce::MidiFile& midi)
{
    int n = 0;
    for (int t = 0; t < midi.getNumTracks(); ++t)
        n += midi.getTrack(t)->getNumEvents();
    return n;
}

} // namespace

class MidiExporterTests : public juce::UnitTest {
public:
    MidiExporterTests() : juce::UnitTest("MidiExporter", "Export") {}

    void runTest() override
    {
        // A test must be open before the first expect(); the runner has no current result otherwise
        // (this suite only passed before because it ran after another one).
        beginTest("scratch directory can be created");

        const juce::File dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                   .getNonexistentChildFile("ChordFoundryMidiExporterTests", "", false);
        expect(dir.createDirectory().wasOk());

        // Progression A: two plain chords.
        const std::vector<ChordData> chordsA { ChordData("I"), ChordData("V") };
        const std::vector<BlockData> blocksA { makeBlock(0, 0, 4), makeBlock(1, 4, 4) };

        // Progression B: three chords, one arpeggiated, so its file differs in size from A.
        ChordData arpChord("vi");
        arpChord.arpMode = "Up";
        arpChord.arpLength = "1/8";
        const std::vector<ChordData> chordsB { ChordData("IV"), ChordData("ii"), arpChord };
        const std::vector<BlockData> blocksB { makeBlock(0, 0, 4), makeBlock(1, 4, 4), makeBlock(2, 8, 8) };

        beginTest("exporting over an existing file replaces it instead of appending");
        {
            const auto target = dir.getChildFile("overwrite.mid");
            const auto fresh = dir.getChildFile("fresh.mid");

            expect(MidiExporter::exportToFile(fresh, blocksB, chordsB, 120.0f, "C", "Major"));
            expect(MidiExporter::exportToFile(target, blocksA, chordsA, 120.0f, "C", "Major"));
            expect(MidiExporter::exportToFile(target, blocksB, chordsB, 120.0f, "C", "Major"));

            expect(fresh.getSize() > 0);
            expectEquals(target.getSize(), fresh.getSize());

            juce::MidiFile midi;
            expect(readBack(target, midi));
            expectEquals(midi.getNumTracks(), 1);

            juce::MidiFile freshMidi;
            expect(readBack(fresh, freshMidi));
            expectEquals(countEvents(midi), countEvents(freshMidi));

            // Byte-for-byte identical to a single fresh export of the same progression.
            juce::MemoryBlock a, b;
            expect(target.loadFileAsData(a));
            expect(fresh.loadFileAsData(b));
            expect(a == b);
        }

        beginTest("export to an unwritable location reports failure");
        {
            const auto bad = dir.getChildFile("no_such_dir").getChildFile("x.mid");
            expect(! MidiExporter::exportToFile(bad, blocksA, chordsA, 120.0f, "C", "Major"));
        }

        beginTest("arpeggiated chord with no notes (invalid roman numeral) does not crash");
        {
            ChordData bogus("not-a-numeral");
            bogus.arpMode = "Up";
            const std::vector<ChordData> chords { bogus };
            const std::vector<BlockData> blocks { makeBlock(0, 0, 8) };

            const auto file = dir.getChildFile("empty_chord.mid");
            expect(MidiExporter::exportToFile(file, blocks, chords, 120.0f, "C", "Major"));

            juce::MidiFile midi;
            expect(readBack(file, midi));
            expectEquals(midi.getNumTracks(), 1);

            // No note events, so the track holds only the tempo (and end-of-track) meta events.
            int noteEvents = 0;
            if (auto* track = midi.getTrack(0))
                for (int i = 0; i < track->getNumEvents(); ++i)
                    if (track->getEventPointer(i)->message.isNoteOnOrOff())
                        ++noteEvents;
            expectEquals(noteEvents, 0);
        }

        beginTest("zero or negative tempo falls back to 120 BPM");
        {
            for (float badTempo : { 0.0f, -60.0f })
            {
                const auto file = dir.getChildFile("tempo.mid");
                expect(MidiExporter::exportToFile(file, blocksA, chordsA, badTempo, "C", "Major"));

                juce::MidiFile midi;
                expect(readBack(file, midi));

                double secondsPerQuarter = 0.0;
                if (auto* track = midi.getTrack(0))
                    for (int i = 0; i < track->getNumEvents(); ++i)
                        if (track->getEventPointer(i)->message.isTempoMetaEvent())
                            secondsPerQuarter = track->getEventPointer(i)->message.getTempoSecondsPerQuarterNote();

                expectWithinAbsoluteError(secondsPerQuarter, 0.5, 1.0e-6);
            }
        }

        dir.deleteRecursively();
    }
};

static MidiExporterTests midiExporterTestsInstance;
