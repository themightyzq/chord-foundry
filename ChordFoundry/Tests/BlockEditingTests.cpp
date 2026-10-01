#include "../Source/Data/ChordProgression.h"

#include <juce_core/juce_core.h>

using namespace ChordFoundry;

namespace {

BlockData makeBlock(int chordIndex, int start, int length)
{
    return BlockData(chordIndex, start, length, juce::Colours::grey);
}

} // namespace

// The rules behind drawing, moving and resizing blocks in the pattern editor. The editor asks
// these questions while the mouse is dragged and then hands the finished block to the model.
class BlockEditingTests : public juce::UnitTest
{
public:
    BlockEditingTests() : juce::UnitTest("BlockEditing", "Data") {}

    void runTest() override
    {
        beginTest("a moved block stays on the 32-step grid");
        {
            const auto block = makeBlock(0, 10, 6);
            expectEquals(ChordProgression::clampMoveStart(block, 12), 12);
            expectEquals(ChordProgression::clampMoveStart(block, -5), 0);
            expectEquals(ChordProgression::clampMoveStart(block, 31), 26, "the whole block fits: 26 + 6 = 32");
            expectEquals(ChordProgression::clampMoveStart(makeBlock(0, 0, 32), 7), 0);
        }

        beginTest("blocks of the same chord may not overlap; different chords may");
        {
            const std::vector<BlockData> blocks { makeBlock(0, 0, 4), makeBlock(0, 8, 4), makeBlock(1, 2, 8) };

            expect(ChordProgression::isPlacementFree(blocks, 0, makeBlock(0, 3, 4)), "moving block 0 to 3..6 is free");
            expect(! ChordProgression::isPlacementFree(blocks, 0, makeBlock(0, 6, 4)), "6..9 runs into the block at 8");
            expect(ChordProgression::isPlacementFree(blocks, 0, makeBlock(0, 4, 4)), "touching is not overlapping");
            expect(! ChordProgression::isPlacementFree(blocks, -1, makeBlock(0, 3, 2)), "a new block cannot sit on block 0");
            expect(ChordProgression::isPlacementFree(blocks, -1, makeBlock(2, 0, 32)), "another chord can lie across everything");
        }

        beginTest("a block can grow until the next block of its chord or the end of the grid");
        {
            const std::vector<BlockData> blocks { makeBlock(0, 0, 4), makeBlock(0, 8, 4), makeBlock(1, 2, 2), makeBlock(0, 28, 2) };

            expectEquals(ChordProgression::maxResizeLength(blocks, 0), 8, "up to the block at 8");
            expectEquals(ChordProgression::maxResizeLength(blocks, 1), 20, "up to the block at 28");
            expectEquals(ChordProgression::maxResizeLength(blocks, 2), 30, "a different chord is not in the way");
            expectEquals(ChordProgression::maxResizeLength(blocks, 3), 4, "up to the end of the grid");
            expectEquals(ChordProgression::maxResizeLength(blocks, 9), 0, "no such block");
        }

        beginTest("a drawn block covers the dragged steps and stops before a block of the same chord");
        {
            const std::vector<BlockData> blocks { makeBlock(0, 8, 4), makeBlock(1, 2, 2) };

            expectEquals(ChordProgression::freeRunLength(blocks, 0, 2, 4), 4, "2..5 is free");
            expectEquals(ChordProgression::freeRunLength(blocks, 0, 5, 10), 3, "stops at step 8");
            expectEquals(ChordProgression::freeRunLength(blocks, 0, 9, 3), 0, "the first step is already taken");
            expectEquals(ChordProgression::freeRunLength(blocks, 1, 5, 10), 10, "other chord's blocks do not count");
            expectEquals(ChordProgression::freeRunLength(blocks, 0, 30, 10), 2, "stops at the end of the grid");
        }

        beginTest("addBlock refuses a same-chord overlap and accepts a layered chord");
        {
            ChordProgression p;
            p.addChord(ChordData("I"));
            p.addChord(ChordData("V"));
            p.addBlock(makeBlock(0, 0, 4));
            p.addBlock(makeBlock(0, 2, 4));
            expectEquals(p.getBlockCount(), 1, "overlapping block refused");
            p.addBlock(makeBlock(1, 2, 4));
            expectEquals(p.getBlockCount(), 2, "a different chord can overlap");
            p.addBlock(makeBlock(0, 4, 4));
            expectEquals(p.getBlockCount(), 3, "a neighbour is fine");
        }

        beginTest("a drag, committed through replaceBlock, moves and resizes the model's block");
        {
            ChordProgression p;
            p.addChord(ChordData("I"));
            p.addBlock(makeBlock(0, 0, 4));
            p.addBlock(makeBlock(0, 10, 4));

            // Move block 0 by +5 steps.
            auto moved = p.getBlocks()[0];
            moved.startStep = ChordProgression::clampMoveStart(moved, moved.startStep + 5);
            expect(ChordProgression::isPlacementFree(p.getBlocks(), 0, moved));
            p.replaceBlock(0, moved);
            expectEquals(p.getBlocks()[0].startStep, 5);

            // Moving it into the other block is not free, so the editor leaves it where it was.
            auto blocked = p.getBlocks()[0];
            blocked.startStep = 8;
            expect(! ChordProgression::isPlacementFree(p.getBlocks(), 0, blocked));
            p.replaceBlock(0, blocked);   // and the model refuses it too
            expectEquals(p.getBlocks()[0].startStep, 5);

            // Resize it as far as it will go.
            auto resized = p.getBlocks()[0];
            resized.lengthSteps = ChordProgression::maxResizeLength(p.getBlocks(), 0);
            p.replaceBlock(0, resized);
            expectEquals(p.getBlocks()[0].lengthSteps, 5, "5..9, up to the block at 10");
        }
    }
};

static BlockEditingTests blockEditingTestsInstance;
