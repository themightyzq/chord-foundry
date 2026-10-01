#include "../Source/Audio/StepSequencer.h"
#include "../Source/Audio/ChordSynthesizer.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <initializer_list>
#include <vector>

using namespace ChordFoundry;

namespace {

using Event = StepSequencer::Event;

struct Recorded
{
    Event event;
    juce::int64 position = 0;   // absolute sample position of the event
};

SequencerSlot makeSlot(std::initializer_list<int> notes, int arpSteps = 0, bool strike = false)
{
    SequencerSlot slot;
    for (const int n : notes)
        slot.notes[slot.numNotes++] = static_cast<juce::uint8>(n);
    slot.arpSteps = arpSteps;
    slot.strike = strike;
    return slot;
}

void put(SequencerPattern& pattern, int step, const SequencerSlot& slot)
{
    auto& target = pattern.steps[step];
    target.slots[target.numSlots++] = slot;
}

void putRange(SequencerPattern& pattern, int first, int count, const SequencerSlot& slot)
{
    for (int s = first; s < first + count; ++s)
        put(pattern, s, slot);
}

// Drives the sequencer the way ChordSynthAudioSource does: blocks of blockSize, each block
// serviced in as many process() calls as it takes, recording every event with its absolute
// sample position.
std::vector<Recorded> run(StepSequencer& seq, juce::int64 totalSamples, int blockSize,
                          juce::int64 startPosition = 0)
{
    std::vector<Recorded> out;
    juce::int64 blockStart = startPosition;

    while (blockStart < startPosition + totalSamples)
    {
        const int want = static_cast<int>(juce::jmin<juce::int64>(blockSize, startPosition + totalSamples - blockStart));
        int done = 0;

        while (done < want)
        {
            const int consumed = seq.process(want - done);
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                Recorded r;
                r.event = seq.getEvents()[i];
                r.position = blockStart + done + r.event.sampleOffset;
                out.push_back(r);
            }
            done += consumed;
        }

        blockStart += want;
    }

    return out;
}

std::vector<juce::int64> stepStarts(const std::vector<Recorded>& events)
{
    std::vector<juce::int64> out;
    for (const auto& r : events)
        if (r.event.type == Event::Type::stepStarted)
            out.push_back(r.position);
    return out;
}

std::vector<Recorded> ofType(const std::vector<Recorded>& events, Event::Type type)
{
    std::vector<Recorded> out;
    for (const auto& r : events)
        if (r.event.type == type)
            out.push_back(r);
    return out;
}

juce::int64 expectedStepPosition(juce::int64 n, double bpm, double rate)
{
    return static_cast<juce::int64>(std::llround(static_cast<double>(n) * 60.0 / bpm / 4.0 * rate));
}

SequencerPattern emptyPattern()
{
    SequencerPattern p;
    p.clear();
    return p;
}

// A pattern that plays something on every step, so every step marker is exercised.
SequencerPattern everyStepPattern()
{
    auto p = emptyPattern();
    for (int s = 0; s < SequencerPattern::numSteps; ++s)
        put(p, s, makeSlot({ 60 + (s % 12) }));
    return p;
}

} // namespace

class StepSequencerTests : public juce::UnitTest
{
public:
    StepSequencerTests() : juce::UnitTest("StepSequencer", "Audio") {}

    void runTest() override
    {
        beginTest("120 BPM at 48 kHz: step n starts at sample n * 6000, whatever the block size");
        {
            const int blockSizes[] = { 64, 128, 333, 480, 512, 1000, 1023, 1024, 4096 };

            for (const int blockSize : blockSizes)
            {
                StepSequencer seq;
                seq.prepare(48000.0);
                seq.setTempo(120.0f);
                seq.setLoop(false);
                seq.setPattern(everyStepPattern());
                seq.startNow();

                const auto events = run(seq, 32 * 6000 + 10, blockSize);
                const auto starts = stepStarts(events);

                expectEquals(static_cast<int>(starts.size()), 32,
                             "32 steps at block size " + juce::String(blockSize));

                bool allExact = true;
                for (size_t n = 0; n < starts.size(); ++n)
                    allExact = allExact && starts[n] == static_cast<juce::int64>(n) * 6000;
                expect(allExact, "every step on an exact multiple of 6000 samples at block size " + juce::String(blockSize));
            }
        }

        beginTest("fractional step lengths round once per step and never drift");
        {
            struct Case { double rate; float bpm; };
            const Case cases[] = { { 44100.0, 137.0f }, { 96000.0, 93.0f }, { 48000.0, 61.0f }, { 44100.0, 240.0f }, { 44100.0, 40.0f } };

            for (const auto& c : cases)
            {
                const int blockSizes[] = { 64, 333, 1024 };
                std::vector<juce::int64> reference;

                for (const int blockSize : blockSizes)
                {
                    StepSequencer seq;
                    seq.prepare(c.rate);
                    seq.setTempo(c.bpm);
                    seq.setLoop(true);
                    seq.setPattern(everyStepPattern());
                    seq.startNow();

                    // 500 steps (about 15 bars): a timer that loses a millisecond per step would
                    // be hundreds of samples out by now.
                    const juce::int64 total = expectedStepPosition(499, c.bpm, c.rate) + 5;
                    const auto starts = stepStarts(run(seq, total, blockSize));

                    expectEquals(static_cast<int>(starts.size()), 500,
                                 "500 steps at " + juce::String(c.bpm) + " BPM, " + juce::String(c.rate) + " Hz");

                    bool exact = true;
                    for (size_t n = 0; n < starts.size(); ++n)
                        exact = exact && starts[n] == expectedStepPosition(static_cast<juce::int64>(n), c.bpm, c.rate);
                    expect(exact, "step positions equal round(n * 60 / bpm / 4 * rate) at block size " + juce::String(blockSize));

                    if (reference.empty())
                        reference = starts;
                    else
                        expect(starts == reference, "positions do not depend on block size");
                }
            }
        }

        beginTest("a non-looping pattern ends at the end of step 32 and releases its notes");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 28, 4, makeSlot({ 60, 64, 67 }));   // sounds through the last step
            seq.setPattern(p);
            seq.startNow();

            const auto events = run(seq, 40 * 6000, 777);
            const auto finished = ofType(events, Event::Type::finished);

            expectEquals(static_cast<int>(finished.size()), 1, "exactly one finished event");
            if (! finished.empty())
                expectEquals(static_cast<int>(finished[0].position), 32 * 6000, "finished lands at the end of step 32");

            const auto offs = ofType(events, Event::Type::noteOff);
            expectEquals(static_cast<int>(offs.size()), 3, "all three notes released");
            for (const auto& off : offs)
                expectEquals(static_cast<int>(off.position), 32 * 6000, "release at the end of the last step");

            expect(! seq.isPlaying(), "not playing after the end");
            expectEquals(seq.getFinishedCount(), 1);
        }

        beginTest("a looping pattern keeps its exact grid across the wrap");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(true);
            seq.setPattern(everyStepPattern());
            seq.startNow();

            const auto starts = stepStarts(run(seq, 70 * 6000, 512));
            expectEquals(static_cast<int>(starts.size()), 70);
            expectEquals(static_cast<int>(starts[32]), 32 * 6000);
            expectEquals(static_cast<int>(starts[64]), 64 * 6000);
            expect(seq.isPlaying());
        }

        beginTest("a tempo change takes effect from the current step without moving earlier steps");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(true);
            seq.setPattern(everyStepPattern());
            seq.startNow();

            // 2.5 steps at 120 BPM, then double the tempo (step length 3000).
            auto events = run(seq, 15000, 500);
            seq.setTempo(240.0f);
            const auto later = run(seq, 20000, 500, 15000);
            events.insert(events.end(), later.begin(), later.end());

            const auto starts = stepStarts(events);
            expect(starts.size() > 6);
            expectEquals(static_cast<int>(starts[0]), 0);
            expectEquals(static_cast<int>(starts[1]), 6000);
            expectEquals(static_cast<int>(starts[2]), 12000);
            // Step 2 began at 12000; the new length (3000) applies to it, so step 3 starts at 15000.
            expectEquals(static_cast<int>(starts[3]), 15000);
            expectEquals(static_cast<int>(starts[4]), 18000);
            expectEquals(static_cast<int>(starts[5]), 21000);
        }

        beginTest("event buffer overflow splits the block but not the grid");
        {
            // Eight slots of sixteen notes on every step, at the fastest tempo, in one huge block.
            auto p = emptyPattern();
            for (int s = 0; s < SequencerPattern::numSteps; ++s)
                for (int slot = 0; slot < SequencerPattern::maxSlots; ++slot)
                {
                    SequencerSlot big;
                    for (int n = 0; n < SequencerSlot::maxNotes; ++n)
                        big.notes[big.numNotes++] = static_cast<juce::uint8>(20 + slot * 8 + n + (s % 2));
                    put(p, s, big);
                }

            std::vector<juce::int64> reference;
            for (const int blockSize : { 64, 100000, 400000 })
            {
                StepSequencer seq;
                seq.prepare(44100.0);
                seq.setTempo(240.0f);
                seq.setLoop(true);
                seq.setPattern(p);
                seq.startNow();

                const auto starts = stepStarts(run(seq, 400000, blockSize));
                expect(starts.size() > 100);

                if (reference.empty())
                    reference = starts;
                else
                    expect(starts == reference, "same step grid at block size " + juce::String(blockSize));
            }
        }

        beginTest("a chord held over 4 steps is struck once and released once");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 4, 4, makeSlot({ 60, 64, 67 }));
            seq.setPattern(p);
            seq.startNow();

            const auto events = run(seq, 12 * 6000, 480);
            const auto ons = ofType(events, Event::Type::noteOn);
            const auto offs = ofType(events, Event::Type::noteOff);

            expectEquals(static_cast<int>(ons.size()), 3, "3 notes struck once, not 12");
            for (const auto& on : ons)
                expectEquals(static_cast<int>(on.position), 4 * 6000, "struck at step 4");

            expectEquals(static_cast<int>(offs.size()), 3);
            for (const auto& off : offs)
                expectEquals(static_cast<int>(off.position), 8 * 6000, "released when the block ends");
        }

        beginTest("the same chord on consecutive steps sustains; a different chord re-strikes");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 0, 2, makeSlot({ 60, 64, 67 }));   // C
            putRange(p, 2, 2, makeSlot({ 60, 64, 67 }));   // C again, separate block: still one strike
            putRange(p, 4, 2, makeSlot({ 62, 65, 69 }));   // Dm: changes
            seq.setPattern(p);
            seq.startNow();

            const auto events = run(seq, 8 * 6000, 512);
            const auto ons = ofType(events, Event::Type::noteOn);
            expectEquals(static_cast<int>(ons.size()), 6, "C struck once (3) and Dm struck once (3)");

            int atStep4 = 0;
            for (const auto& on : ons)
                if (on.position == 4 * 6000)
                    ++atStep4;
            expectEquals(atStep4, 3, "the change to Dm strikes on step 4");

            // Note-offs for C go out before the note-ons for Dm at the same sample.
            int lastOffIndex = -1, firstDmOnIndex = -1;
            for (size_t i = 0; i < events.size(); ++i)
            {
                if (events[i].event.type == Event::Type::noteOff && events[i].position == 4 * 6000)
                    lastOffIndex = static_cast<int>(i);
                if (events[i].event.type == Event::Type::noteOn && events[i].position == 4 * 6000 && firstDmOnIndex < 0)
                    firstDmOnIndex = static_cast<int>(i);
            }
            expect(lastOffIndex >= 0 && lastOffIndex < firstDmOnIndex, "note-offs precede note-ons");
        }

        beginTest("a slot flagged as a new strike re-strikes even on the same chord");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 0, 2, makeSlot({ 60, 64, 67 }));
            put(p, 2, makeSlot({ 60, 64, 67 }, 0, true));   // flagged: strike again here...
            put(p, 3, makeSlot({ 60, 64, 67 }));            // ...and sustain from there
            seq.setPattern(p);
            seq.startNow();

            const auto ons = ofType(run(seq, 6 * 6000, 512), Event::Type::noteOn);
            expectEquals(static_cast<int>(ons.size()), 6, "two strikes of three notes");
            expectEquals(static_cast<int>(ons.back().position), 2 * 6000);
        }

        beginTest("an arpeggio keeps its place across steps and across block boundaries");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            // One arp note every 2 steps, over 8 steps made of two adjacent blocks of the same chord.
            auto p = emptyPattern();
            putRange(p, 0, 4, makeSlot({ 60, 64, 67 }, 2));
            putRange(p, 4, 4, makeSlot({ 60, 64, 67 }, 2));
            seq.setPattern(p);
            seq.startNow();

            const auto ons = ofType(run(seq, 10 * 6000, 1024), Event::Type::noteOn);
            expectEquals(static_cast<int>(ons.size()), 4, "notes at steps 0, 2, 4, 6");

            const int expectedNotes[] = { 60, 64, 67, 60 };
            for (size_t i = 0; i < ons.size() && i < 4; ++i)
            {
                expectEquals(ons[i].event.note, expectedNotes[i], "arp note " + juce::String(static_cast<int>(i)));
                expectEquals(static_cast<int>(ons[i].position), static_cast<int>(i) * 2 * 6000);
            }
        }

        beginTest("an arpeggio restarts when the chord changes");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 0, 3, makeSlot({ 60, 64, 67 }, 1));  // C arp, a note per step: 60 64 67
            putRange(p, 3, 3, makeSlot({ 62, 65, 69 }, 1));  // Dm arp restarts at its first note
            seq.setPattern(p);
            seq.startNow();

            const auto ons = ofType(run(seq, 8 * 6000, 512), Event::Type::noteOn);
            expectEquals(static_cast<int>(ons.size()), 6);
            const int expectedNotes[] = { 60, 64, 67, 62, 65, 69 };
            for (size_t i = 0; i < ons.size() && i < 6; ++i)
                expectEquals(ons[i].event.note, expectedNotes[i]);
        }

        beginTest("replacing the pattern while playing re-strikes only what changed");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(true);

            auto p = emptyPattern();
            putRange(p, 0, 32, makeSlot({ 60, 64, 67 }));
            seq.setPattern(p);
            seq.startNow();

            auto events = run(seq, 3 * 6000, 512);

            // Same chord again: nothing re-struck.
            seq.setPattern(p);
            auto more = run(seq, 3 * 6000, 512, 3 * 6000);
            events.insert(events.end(), more.begin(), more.end());
            expectEquals(static_cast<int>(ofType(events, Event::Type::noteOn).size()), 3, "same pattern: still one strike");

            // Different chord from now on: struck again.
            auto changed = emptyPattern();
            putRange(changed, 0, 32, makeSlot({ 62, 65, 69 }));
            seq.setPattern(changed);
            more = run(seq, 3 * 6000, 512, 6 * 6000);
            events.insert(events.end(), more.begin(), more.end());
            expectEquals(static_cast<int>(ofType(events, Event::Type::noteOn).size()), 6, "chord change: struck again");
        }

        beginTest("click events land on step boundaries with an accent every fourth step");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(false);
            seq.setClickEnabled(true);
            seq.setPattern(emptyPattern());
            seq.startNow();

            const auto clicks = ofType(run(seq, 8 * 6000, 300), Event::Type::click);
            expectEquals(static_cast<int>(clicks.size()), 8);
            for (size_t n = 0; n < clicks.size(); ++n)
            {
                expectEquals(static_cast<int>(clicks[n].position), static_cast<int>(n) * 6000);
                expect(clicks[n].event.accent == (n % 4 == 0), "accent on steps 1 and 5");
            }
        }

        beginTest("stopNow silences the scheduler and a new start begins again at step 0");
        {
            StepSequencer seq;
            seq.prepare(48000.0);
            seq.setTempo(120.0f);
            seq.setLoop(true);
            seq.setPattern(everyStepPattern());
            seq.startNow();
            run(seq, 3 * 6000, 512);

            seq.stopNow();
            expect(! seq.isPlaying());
            expectEquals(static_cast<int>(run(seq, 6000, 512, 3 * 6000).size()), 0, "no events while stopped");

            seq.startNow();
            const auto starts = stepStarts(run(seq, 2 * 6000, 512, 4 * 6000));
            expect(! starts.empty());
            expectEquals(static_cast<int>(starts[0]), 4 * 6000, "starts at the first sample after the restart");
            expectEquals(seq.getCurrentStep(), 1);
        }

        runAudioSourceTests();
    }

private:
    // The same guarantees, measured on rendered audio through ChordSynthAudioSource.
    void runAudioSourceTests()
    {
        constexpr double rate = 48000.0;

        beginTest("rendered click track: step n's click starts at sample n * 6000 (120 BPM, 48 kHz)");
        {
            for (const int blockSize : { 64, 333, 480, 1024 })
            {
                ChordSynthAudioSource source;
                source.prepareToPlay(blockSize, rate);
                source.setTempo(120.0f);
                source.setLoop(false);
                source.setSequencerClickEnabled(true);
                source.setSequencerPattern(emptyPattern());
                source.startSequencer();

                const int total = 8 * 6000;
                juce::AudioBuffer<float> out(2, total);
                out.clear();

                for (int done = 0; done < total; done += blockSize)
                {
                    const int n = juce::jmin(blockSize, total - done);
                    juce::AudioSourceChannelInfo info(&out, done, n);
                    source.getNextAudioBlock(info);
                }

                // A click is a 2 ms burst whose first sample is sin(0) = 0, so the first
                // non-zero sample of each burst is its start + 1.
                const float* data = out.getReadPointer(0);
                std::vector<int> onsets;
                for (int i = 1; i < total; ++i)
                    if (data[i] != 0.0f && data[i - 1] == 0.0f && (onsets.empty() || i - onsets.back() > 1000))
                        onsets.push_back(i);

                expectEquals(static_cast<int>(onsets.size()), 8, "8 clicks at block size " + juce::String(blockSize));
                for (size_t n = 0; n < onsets.size(); ++n)
                    expectEquals(onsets[n], static_cast<int>(n) * 6000 + 1,
                                 "click " + juce::String(static_cast<int>(n)) + " at block size " + juce::String(blockSize));
            }
        }

        beginTest("rendered sequence: a held chord is struck once and sustains through every step");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(512, rate);
            source.setTempo(120.0f);
            source.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 0, 4, makeSlot({ 60, 64, 67 }));
            source.setSequencerPattern(p);
            source.startSequencer();

            const int total = 4 * 6000;
            juce::AudioBuffer<float> out(2, total);
            out.clear();
            for (int done = 0; done < total; done += 512)
            {
                juce::AudioSourceChannelInfo info(&out, done, juce::jmin(512, total - done));
                source.getNextAudioBlock(info);
            }

            expectEquals(source.getNoteOnCount(), 3, "three note-ons for four steps, not twelve");
            expectEquals(source.getSequencerStep(), 3);

            // Still sounding in the last step, long after the single strike.
            const float firstStep = out.getMagnitude(0, 3000, 2000);
            const float lastStep = out.getMagnitude(0, 3 * 6000 + 1000, 4000);
            expect(firstStep > 0.01f && lastStep > 0.01f, "sounding in the first and last step");
        }

        beginTest("rendered sequence ends by itself and the notes decay");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(512, rate);
            source.setTempo(240.0f);
            source.setLoop(false);

            auto p = emptyPattern();
            putRange(p, 30, 2, makeSlot({ 60 }));
            source.setSequencerPattern(p);
            source.startSequencer();

            const int total = 32 * 3000 + 20000;
            juce::AudioBuffer<float> out(2, total);
            out.clear();
            for (int done = 0; done < total; done += 512)
            {
                juce::AudioSourceChannelInfo info(&out, done, juce::jmin(512, total - done));
                source.getNextAudioBlock(info);
            }

            expectEquals(source.getSequencerFinishedCount(), 1);
            expect(! source.isSequencerPlaying());
            expect(out.getMagnitude(0, total - 4000, 4000) < 1.0e-4f, "silent well after the end");
        }
    }
};

static StepSequencerTests stepSequencerTestsInstance;
