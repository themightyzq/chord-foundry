#include "../Source/Data/ProjectFile.h"

#include <juce_core/juce_core.h>

using namespace ChordFoundry;

namespace {

// A project that uses every field the file format carries, so a field that is dropped or
// mangled on the way through shows up as a difference.
void buildRichProject(ProjectSettings& settings, ChordProgression& progression)
{
    settings.tempo = 133.3f;
    settings.key = "F#/Gb";
    settings.mode = "Dorian";
    settings.loop = true;
    settings.clickTrack = true;
    settings.masterVolume = 0.37f;

    ChordData a("I");
    a.extension = "7";
    a.inversion = "1st";
    a.voicing = "Drop 2";
    a.arpMode = "Converge";
    a.arpLength = "1/8";
    a.customVoicing = { 5, 2, 4 };
    progression.addChord(a);

    ChordData b("vi");
    b.extension = "9";
    b.voicing = "Custom";
    b.customVoicing = { 4, 5, 1 };
    progression.addChord(b);

    progression.addChord(ChordData("V"));

    BlockData first(0, 0, 4, juce::Colour(0xff112233));
    progression.addBlock(first);

    BlockData second(1, 4, 8, juce::Colour(0xffaa5500));
    second.hasModifierOverrides = true;
    second.modifierOverrides.extension = "sus4";
    second.modifierOverrides.inversion = "2nd";
    second.modifierOverrides.voicing = "Custom";
    second.modifierOverrides.arpMode = "Diverge";
    second.modifierOverrides.arpLength = "1/4";
    second.modifierOverrides.customVoicing = { 6, 1, 3 };
    second.newStrike = true;
    progression.addBlock(second);

    progression.addBlock(BlockData(2, 20, 2, juce::Colours::transparentBlack));
}

bool sameModel(const ProjectSettings& sa, const ChordProgression& pa,
               const ProjectSettings& sb, const ChordProgression& pb)
{
    return sa == sb && pa.getChords() == pb.getChords() && pa.getBlocks() == pb.getBlocks();
}

} // namespace

class ProjectFileTests : public juce::UnitTest
{
public:
    ProjectFileTests() : juce::UnitTest("ProjectFile", "Data") {}

    void runTest() override
    {
        beginTest("scratch directory can be created");
        const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getNonexistentChildFile("ChordFoundryProjectFileTests", "", false);
        expect(dir.createDirectory().wasOk());

        ProjectSettings original;
        ChordProgression originalProgression;
        buildRichProject(original, originalProgression);
        expectEquals(originalProgression.getChordCount(), 3);
        expectEquals(originalProgression.getBlockCount(), 3);

        beginTest("save then load gives an identical model");
        {
            const auto file = dir.getChildFile("roundtrip.cfproj");
            const auto saved = ProjectFile::save(file, original, originalProgression);
            expect(saved.wasOk(), saved.getErrorMessage());
            expect(file.existsAsFile());

            ProjectSettings loaded;
            ChordProgression loadedProgression;
            const auto result = ProjectFile::load(file, loaded, loadedProgression);
            expect(result.wasOk(), result.getErrorMessage());

            expect(loaded == original, "settings identical");
            expect(loadedProgression.getChords() == originalProgression.getChords(), "chords identical");
            expect(loadedProgression.getBlocks() == originalProgression.getBlocks(), "blocks identical");

            // Spot checks on the fields that are easiest to lose.
            expectEquals(loaded.tempo, 133.3f);
            expectEquals(loaded.masterVolume, 0.37f);
            expect(loadedProgression.getBlocks()[1].newStrike, "new-strike flag kept");
            expectEquals(loadedProgression.getBlocks()[1].modifierOverrides.customVoicing.numNotes, 6);
            expectEquals(loadedProgression.getBlocks()[1].modifierOverrides.customVoicing.spreadType, 3);
            expectEquals(loadedProgression.getChord(0).customVoicing.position, 2);
            expectEquals(loadedProgression.getChord(0).arpMode, juce::String("Converge"));
        }

        beginTest("a second round trip changes nothing (text is stable)");
        {
            const auto text1 = ProjectFile::toXmlString(original, originalProgression);

            ProjectSettings s2;
            ChordProgression p2;
            expect(ProjectFile::parse(text1, s2, p2).wasOk());
            expect(ProjectFile::toXmlString(s2, p2) == text1, "same text after load and save");
            expectEquals(ProjectFile::toXmlString(original, originalProgression), text1);
        }

        beginTest("the file carries a format version");
        {
            const auto text = ProjectFile::toXmlString(original, originalProgression);
            expect(text.contains("ChordFoundryProject"));
            expect(text.contains("formatVersion=\"" + juce::String(ProjectFile::currentFormatVersion) + "\""));
        }

        beginTest("saving over an existing project replaces it and leaves no temporary file behind");
        {
            const auto file = dir.getChildFile("replace.cfproj");
            expect(file.replaceWithText(juce::String::repeatedString("old project bytes ", 500)));

            expect(ProjectFile::save(file, original, originalProgression).wasOk());

            ProjectSettings loaded;
            ChordProgression loadedProgression;
            expect(ProjectFile::load(file, loaded, loadedProgression).wasOk(), "new content is valid");
            expect(sameModel(original, originalProgression, loaded, loadedProgression));
            expect(file.getSize() < 500 * 18 + 20000, "old bytes are gone, not appended to");

            int leftovers = 0;
            for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "replace*"))
                if (f != file)
                    ++leftovers;
            expectEquals(leftovers, 0, "no temp files left next to the project");
        }

        beginTest("a save that fails leaves the existing file untouched");
        {
            // Target folder does not exist.
            const auto missing = dir.getChildFile("no_such_folder").getChildFile("x.cfproj");
            expect(ProjectFile::save(missing, original, originalProgression).failed());
            expect(! missing.existsAsFile());

            // Read-only folder with a good project in it.
            const auto lockedDir = dir.getChildFile("locked");
            expect(lockedDir.createDirectory().wasOk());
            const auto file = lockedDir.getChildFile("keep.cfproj");
            expect(ProjectFile::save(file, original, originalProgression).wasOk());
            const auto before = file.loadFileAsString();

            lockedDir.setReadOnly(true, false);
            const auto probe = lockedDir.getChildFile("probe.tmp");
            const bool folderIsLocked = ! probe.create().wasOk();   // false when running as root
            probe.deleteFile();

            if (folderIsLocked)
            {
                ProjectSettings changed = original;
                changed.tempo = 90.0f;
                const auto result = ProjectFile::save(file, changed, originalProgression);
                expect(result.failed(), "save into a read-only folder reports failure");
                expect(! result.getErrorMessage().isEmpty(), "with a message");
                expect(file.loadFileAsString() == before, "existing project is byte-for-byte unchanged");
            }

            lockedDir.setReadOnly(false, false);
        }

        beginTest("loading refuses bad files and leaves the model untouched");
        {
            ProjectSettings keptSettings = original;
            ChordProgression keptProgression;
            ProjectSettings dummy;
            buildRichProject(dummy, keptProgression);

            const auto check = [&](const juce::String& label, const juce::String& text, const juce::String& expectedPart)
            {
                const auto result = ProjectFile::parse(text, keptSettings, keptProgression);
                expect(result.failed(), label + " is refused");
                expect(result.getErrorMessage().containsIgnoreCase(expectedPart),
                       label + ": message mentions '" + expectedPart + "' (got: " + result.getErrorMessage() + ")");
                expect(sameModel(keptSettings, keptProgression, original, originalProgression),
                       label + ": nothing was changed");
            };

            check("empty text", "", "not a Chord Foundry");
            check("not xml", "this is not xml at all", "not a Chord Foundry");
            check("wrong root", "<Other formatVersion=\"1\"/>", "not a Chord Foundry");
            check("no version", "<ChordFoundryProject/>", "format version");
            check("future version",
                  "<ChordFoundryProject formatVersion=\"" + juce::String(ProjectFile::currentFormatVersion + 1) + "\"/>",
                  "newer version");
            check("missing parts", "<ChordFoundryProject formatVersion=\"1\"><Settings tempo=\"100\"/></ChordFoundryProject>",
                  "incomplete");

            const auto missingFile = dir.getChildFile("does_not_exist.cfproj");
            const auto result = ProjectFile::load(missingFile, keptSettings, keptProgression);
            expect(result.failed() && result.getErrorMessage().contains("does not exist"));
        }

        beginTest("out-of-range values are clamped and unknown attributes are ignored");
        {
            const juce::String text =
                "<ChordFoundryProject formatVersion=\"1\" someFutureField=\"x\">"
                "<Settings tempo=\"9000\" key=\"\" mode=\"\" loop=\"1\" clickTrack=\"0\" masterVolume=\"7\" extra=\"1\"/>"
                "<ChordProgression><Chords/><PatternBlocks/></ChordProgression>"
                "</ChordFoundryProject>";

            ProjectSettings s;
            ChordProgression p;
            const auto result = ProjectFile::parse(text, s, p);
            expect(result.wasOk(), result.getErrorMessage());
            expectEquals(s.tempo, 240.0f);
            expectEquals(s.masterVolume, 1.0f);
            expectEquals(s.key, juce::String("C"));
            expectEquals(s.mode, juce::String("Major"));
            expect(s.loop && ! s.clickTrack);
            expectEquals(p.getChordCount(), 0);
        }

        beginTest("blocks that point at a missing chord are dropped on load");
        {
            const juce::String text =
                "<ChordFoundryProject formatVersion=\"1\">"
                "<Settings tempo=\"120\"/>"
                "<ChordProgression><Chords><Chord roman=\"I\"/></Chords>"
                "<PatternBlocks><Block chordIndex=\"0\" startStep=\"0\" lengthSteps=\"4\"/>"
                "<Block chordIndex=\"3\" startStep=\"4\" lengthSteps=\"4\"/>"
                "<Block chordIndex=\"0\" startStep=\"30\" lengthSteps=\"9\"/></PatternBlocks></ChordProgression>"
                "</ChordFoundryProject>";

            ProjectSettings s;
            ChordProgression p;
            expect(ProjectFile::parse(text, s, p).wasOk());
            expectEquals(p.getChordCount(), 1);
            expectEquals(p.getBlockCount(), 1);
        }

        beginTest("at most eight chords are loaded");
        {
            juce::String chords;
            for (int i = 0; i < 12; ++i)
                chords += "<Chord roman=\"I\"/>";

            const juce::String text = "<ChordFoundryProject formatVersion=\"1\"><Settings/>"
                                      "<ChordProgression><Chords>" + chords + "</Chords><PatternBlocks/></ChordProgression>"
                                      "</ChordFoundryProject>";
            ProjectSettings s;
            ChordProgression p;
            expect(ProjectFile::parse(text, s, p).wasOk());
            expectEquals(p.getChordCount(), ChordProgression::MAX_CHORDS);
        }

        beginTest("replaceBlock changes a block and refuses invalid or overlapping replacements");
        {
            ChordProgression p;
            p.addChord(ChordData("I"));
            p.addChord(ChordData("V"));
            p.addBlock(BlockData(0, 0, 4, juce::Colours::grey));
            p.addBlock(BlockData(0, 8, 4, juce::Colours::grey));

            auto changed = p.getBlocks()[0];
            changed.newStrike = true;
            p.replaceBlock(0, changed);
            expect(p.getBlocks()[0].newStrike, "flag set");

            auto overlapping = p.getBlocks()[0];
            overlapping.lengthSteps = 12;   // would run into the block at step 8 (same chord)
            p.replaceBlock(0, overlapping);
            expectEquals(p.getBlocks()[0].lengthSteps, 4, "overlap refused");

            auto outside = p.getBlocks()[1];
            outside.startStep = 31;
            p.replaceBlock(1, outside);
            expectEquals(p.getBlocks()[1].startStep, 8, "out-of-range refused");

            p.replaceBlock(7, changed);   // no such block: ignored
            expectEquals(p.getBlockCount(), 2);
        }

        dir.deleteRecursively();
    }
};

static ProjectFileTests projectFileTestsInstance;
