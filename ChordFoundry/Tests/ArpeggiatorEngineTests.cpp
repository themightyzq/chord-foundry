#include "../Source/Audio/ArpeggiatorEngine.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>

using namespace ChordFoundry;

namespace {

bool nearlyEqual(const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (std::fabs(a[i] - b[i]) > 1.0e-6f)
            return false;

    return true;
}

// True if `a` and `b` contain the same multiset of values, ignoring order.
bool isPermutation(std::vector<float> a, std::vector<float> b)
{
    if (a.size() != b.size())
        return false;

    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return nearlyEqual(a, b);
}

} // namespace

// Exercises every arpeggiator mode ported from archive/core/arpeggiator.py,
// plus getNoteLength's tempo/division mapping and the empty/single-note edge
// cases.
class ArpeggiatorEngineTests : public juce::UnitTest {
public:
    ArpeggiatorEngineTests() : juce::UnitTest("ArpeggiatorEngine", "Audio") {}

    void runTest() override
    {
        ArpeggiatorEngine engine;
        const std::vector<float> notes { 60.0f, 64.0f, 67.0f, 72.0f };

        beginTest("Up returns notes unchanged");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "Up"), notes));

        beginTest("Down reverses notes");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "Down"),
                            { 72.0f, 67.0f, 64.0f, 60.0f }));

        beginTest("None returns notes unchanged");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "None"), notes));

        beginTest("Empty mode string returns notes unchanged");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, ""), notes));

        beginTest("Unrecognised mode returns notes unchanged");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "Bogus"), notes));

        beginTest("Random is a permutation of the input");
        auto randomResult = engine.getArpeggioSequence(notes, "Random");
        expect(isPermutation(randomResult, notes));

        beginTest("Converge is [first, last] followed by the inner notes");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "Converge"),
                            { 60.0f, 72.0f, 64.0f, 67.0f }));

        beginTest("Diverge starts at the middle note and expands outward");
        expect(nearlyEqual(engine.getArpeggioSequence(notes, "Diverge"),
                            { 67.0f, 64.0f, 60.0f, 72.0f }));

        const std::vector<float> unsorted { 67.0f, 60.0f, 72.0f, 64.0f };

        beginTest("Ascending sorts low to high");
        expect(nearlyEqual(engine.getArpeggioSequence(unsorted, "Ascending"),
                            { 60.0f, 64.0f, 67.0f, 72.0f }));

        beginTest("Descending sorts high to low");
        expect(nearlyEqual(engine.getArpeggioSequence(unsorted, "Descending"),
                            { 72.0f, 67.0f, 64.0f, 60.0f }));

        beginTest("Empty input returns empty for every mode");
        const std::vector<float> empty;
        expect(engine.getArpeggioSequence(empty, "Up").empty());
        expect(engine.getArpeggioSequence(empty, "Down").empty());
        expect(engine.getArpeggioSequence(empty, "Converge").empty());
        expect(engine.getArpeggioSequence(empty, "Diverge").empty());
        expect(engine.getArpeggioSequence(empty, "Random").empty());
        expect(engine.getArpeggioSequence(empty, "Ascending").empty());
        expect(engine.getArpeggioSequence(empty, "Descending").empty());

        beginTest("Single note is unchanged by every mode");
        const std::vector<float> single { 60.0f };
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Up"), single));
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Down"), single));
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Converge"), single));
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Diverge"), single));
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Ascending"), single));
        expect(nearlyEqual(engine.getArpeggioSequence(single, "Descending"), single));
        expect(isPermutation(engine.getArpeggioSequence(single, "Random"), single));

        beginTest("getNoteLength maps each division at 120 BPM");
        expect(juce::approximatelyEqual(engine.getNoteLength("1/16", 120.0f), 0.125));
        expect(juce::approximatelyEqual(engine.getNoteLength("1/8", 120.0f), 0.25));
        expect(juce::approximatelyEqual(engine.getNoteLength("1/4", 120.0f), 0.5));
        expect(juce::approximatelyEqual(engine.getNoteLength("1/2", 120.0f), 1.0));

        beginTest("getNoteLength scales inversely with tempo");
        expect(juce::approximatelyEqual(engine.getNoteLength("1/4", 60.0f), 1.0));
        expect(juce::approximatelyEqual(engine.getNoteLength("1/4", 240.0f), 0.25));

        beginTest("getNoteLength defaults an unrecognised division to 1/16");
        expect(juce::approximatelyEqual(engine.getNoteLength("bogus", 120.0f), 0.125));
    }
};

static ArpeggiatorEngineTests arpeggiatorEngineTestsInstance;
