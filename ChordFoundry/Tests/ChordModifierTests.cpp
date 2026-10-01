#include "../Source/Audio/ArpeggiatorEngine.h"
#include "../Source/Audio/SequencerPatternBuilder.h"
#include "../Source/Data/ChordProgression.h"
#include "../Source/Data/ProjectFile.h"
#include "../Source/MusicTheory/MusicTheoryEngine.h"

#include <juce_core/juce_core.h>

using namespace ChordFoundry;

namespace {

std::vector<int> notesOf(const ChordData& chord)
{
    return MusicTheoryEngine::getChordMidiNotes(chord.roman, "C", "Major", chord);
}

} // namespace

// The model path behind the Modifiers dialog: a chord's modifiers are replaced through the
// progression, and from there they reach the sound, the saved file and the dirty check.
class ChordModifierTests : public juce::UnitTest
{
public:
    ChordModifierTests() : juce::UnitTest("ChordModifiers", "Data") {}

    void runTest() override
    {
        beginTest("updateChord replaces a chord's modifiers and refuses a bad index");
        {
            ChordProgression p;
            p.addChord(ChordData("I"));
            p.addChord(ChordData("V"));
            p.addBlock(BlockData(1, 4, 4, juce::Colours::grey));

            ChordData edited("V");
            edited.extension = "+7th";
            edited.inversion = "1st";
            edited.voicing = "Open";
            edited.arpMode = "Down";
            edited.arpLength = "1/4";

            expect(p.updateChord(1, edited));
            expect(p.getChord(1) == edited, "all modifiers stored");
            expect(p.getChord(0) == ChordData("I"), "the other chord is untouched");
            expectEquals(p.getBlockCount(), 1, "blocks still refer to the chord");
            expectEquals(p.getBlocks()[0].chordIndex, 1);

            expect(! p.updateChord(2, edited));
            expect(! p.updateChord(-1, edited));
            expectEquals(p.getChordCount(), 2);
        }

        beginTest("a modified chord changes what is played");
        {
            ChordProgression p;
            p.addChord(ChordData("I"));
            p.addBlock(BlockData(0, 0, 4, juce::Colours::grey));
            const auto before = SequencerPatternBuilder::build(p, "C", "Major");

            ChordData edited("I");
            edited.extension = "+7th";
            p.updateChord(0, edited);
            const auto withSeventh = SequencerPatternBuilder::build(p, "C", "Major");
            expect(withSeventh.steps[0].slots[0].numNotes == before.steps[0].slots[0].numNotes + 1, "a 7th adds a note");
            expect(! withSeventh.steps[0].slots[0].sameVoicing(before.steps[0].slots[0]));

            edited.arpMode = "Up";
            edited.arpLength = "1/4";
            p.updateChord(0, edited);
            const auto arp = SequencerPatternBuilder::build(p, "C", "Major");
            expectEquals(arp.steps[0].slots[0].arpSteps, 4, "the arpeggiator reaches the sequencer");
        }

        beginTest("modifiers are saved, restored, and make the project count as changed");
        {
            ChordProgression p;
            p.addChord(ChordData("ii"));
            p.addBlock(BlockData(0, 0, 8, juce::Colours::grey));
            ProjectSettings settings;
            const auto saved = ProjectFile::toXmlString(settings, p);

            ChordData edited("ii");
            edited.extension = "sus4";
            edited.inversion = "2nd";
            edited.voicing = "Custom";
            edited.arpMode = "Converge";
            edited.arpLength = "1/2";
            edited.customVoicing = { 6, 2, 4 };
            p.updateChord(0, edited);

            const auto changed = ProjectFile::toXmlString(settings, p);
            expect(changed != saved, "the saved text differs, so the project is unsaved");

            ProjectSettings loadedSettings;
            ChordProgression loaded;
            expect(ProjectFile::parse(changed, loadedSettings, loaded).wasOk());
            expect(loaded.getChord(0) == edited, "every modifier survives save and load");
        }

        beginTest("every value the dialog offers is understood by the engine");
        {
            const ChordData plain("I");
            const auto base = notesOf(plain);
            expect(base.size() >= 3);

            for (const auto& extension : MusicTheoryEngine::EXTENSION_NAMES)
            {
                ChordData c(plain);
                c.extension = extension == "None" ? juce::String() : extension;
                expect((notesOf(c) != base) == (extension != "None"), "extension " + extension);
            }

            for (const auto& inversion : MusicTheoryEngine::INVERSION_NAMES)
            {
                ChordData c(plain);
                c.inversion = inversion == "None" ? juce::String() : inversion;
                const bool changes = inversion == "1st" || inversion == "2nd";
                expect((notesOf(c) != base) == changes, "inversion " + inversion);
            }

            for (const auto& voicing : MusicTheoryEngine::VOICING_NAMES)
            {
                ChordData c(plain);
                c.voicing = voicing == "None" ? juce::String() : voicing;
                expect(notesOf(c).size() >= 3, "voicing " + voicing + " still gives a chord");
                if (voicing == "Open" || voicing == "Drop 2")
                    expect(notesOf(c) != base, "voicing " + voicing + " changes the notes");
            }

            for (int spread = 0; spread < static_cast<int>(MusicTheoryEngine::SPREAD_TYPE_NAMES.size()); ++spread)
            {
                ChordData c(plain);
                c.voicing = "Custom";
                c.customVoicing = { 4, 3, spread };
                expect(! notesOf(c).empty(), "custom spread " + MusicTheoryEngine::SPREAD_TYPE_NAMES[static_cast<size_t>(spread)]);
            }

            std::vector<int> stepCounts;
            for (const auto& length : MusicTheoryEngine::ARP_LENGTH_NAMES)
                stepCounts.push_back(ArpeggiatorEngine::getNoteLengthSteps(length));
            for (size_t i = 0; i < stepCounts.size(); ++i)
                for (size_t j = i + 1; j < stepCounts.size(); ++j)
                    expect(stepCounts[i] != stepCounts[j], "each arp length offered is a different speed");

            ArpeggiatorEngine arp;
            const std::vector<float> asFloat { 60.0f, 64.0f, 67.0f };
            for (const auto& mode : MusicTheoryEngine::ARP_MODE_NAMES)
                expectEquals(static_cast<int>(arp.getArpeggioSequence(asFloat, mode, 3u).size()), 3, "arp mode " + mode);
        }
    }
};

static ChordModifierTests chordModifierTestsInstance;
