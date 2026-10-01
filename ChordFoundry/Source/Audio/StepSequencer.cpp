#include "StepSequencer.h"

#include <cmath>
#include <cstring>

namespace ChordFoundry {

//==============================================================================
bool SequencerSlot::sameVoicing (const SequencerSlot& other) const noexcept
{
    if (numNotes != other.numNotes || arpSteps != other.arpSteps || velocity != other.velocity)
        return false;

    return std::memcmp (notes, other.notes, static_cast<size_t> (numNotes)) == 0;
}

void SequencerPattern::clear() noexcept
{
    for (auto& step : steps)
        step.numSlots = 0;
}

//==============================================================================
StepSequencer::StepSequencer() = default;

double StepSequencer::getStepLengthSamples (double bpm, double rate) noexcept
{
    return 60.0 / bpm / 4.0 * rate;
}

void StepSequencer::setTempo (float bpm) noexcept
{
    tempo.store (clampTempo (bpm), std::memory_order_relaxed);
}

void StepSequencer::setPattern (const SequencerPattern& newPattern)
{
    patterns[writeIndex] = newPattern;
    writeIndex = middle.exchange (writeIndex | dirtyBit, std::memory_order_acq_rel) & 3;
}

void StepSequencer::pickUpPattern() noexcept
{
    if ((middle.load (std::memory_order_acquire) & dirtyBit) != 0)
        readIndex = middle.exchange (readIndex, std::memory_order_acq_rel) & 3;
}

//==============================================================================
void StepSequencer::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    stepLength = getStepLengthSamples (currentBpm, sampleRate);

    if (playing)
    {
        // The step grid was laid out for the old rate; end playback rather than guess.
        stopNow();
        finishedCount.fetch_add (1, std::memory_order_relaxed);
    }
}

void StepSequencer::startNow() noexcept
{
    pickUpPattern();

    for (auto& a : active)
        a = ActiveSlot();

    currentBpm = clampTempo (tempo.load (std::memory_order_relaxed));
    stepLength = getStepLengthSamples (currentBpm, sampleRate);
    nextStep = 0;
    baseStep = 0;
    baseSample = static_cast<double> (samplePos);
    playing = true;

    currentStep.store (-1, std::memory_order_relaxed);
    playingFlag.store (true, std::memory_order_relaxed);
}

void StepSequencer::stopNow() noexcept
{
    for (auto& a : active)
        a = ActiveSlot();

    playing = false;
    currentStep.store (-1, std::memory_order_relaxed);
    playingFlag.store (false, std::memory_order_relaxed);
}

//==============================================================================
double StepSequencer::boundaryOf (std::int64_t globalStep) const noexcept
{
    return baseSample + static_cast<double> (globalStep - baseStep) * stepLength;
}

void StepSequencer::applyTempoChange() noexcept
{
    const float bpm = clampTempo (tempo.load (std::memory_order_relaxed));
    if (bpm == currentBpm)
        return;

    // Keep the start of the step that is currently sounding where it is and lay the following
    // steps out at the new tempo, so the change takes effect immediately and without a jump.
    const std::int64_t anchor = juce::jmax (baseStep, nextStep - 1);
    baseSample = boundaryOf (anchor);
    baseStep = anchor;
    currentBpm = bpm;
    stepLength = getStepLengthSamples (bpm, sampleRate);
}

void StepSequencer::push (Event::Type type, int offset, int note, float velocity, bool accent,
                          int step, std::int64_t absoluteStep) noexcept
{
    // process() reserves room for the worst case before it fires a step.
    auto& e = events[numEvents++];
    e.type = type;
    e.sampleOffset = offset;
    e.note = note;
    e.velocity = velocity;
    e.accent = accent;
    e.step = step;
    e.absoluteStep = absoluteStep;
}

void StepSequencer::releaseAll (int offset) noexcept
{
    for (auto& a : active)
    {
        if (! a.used)
            continue;

        if (a.slot.arpSteps > 0)
        {
            if (a.soundingNote >= 0)
                push (Event::Type::noteOff, offset, a.soundingNote, 0.0f, false, 0, 0);
        }
        else
        {
            for (int i = 0; i < a.slot.numNotes; ++i)
                push (Event::Type::noteOff, offset, a.slot.notes[i], 0.0f, false, 0, 0);
        }

        a = ActiveSlot();
    }
}

void StepSequencer::fireStep (std::int64_t globalStep, int offset) noexcept
{
    constexpr int numSteps = SequencerPattern::numSteps;

    if (! loop.load (std::memory_order_relaxed) && globalStep >= numSteps)
    {
        releaseAll (offset);
        push (Event::Type::finished, offset, 0, 0.0f, false, 0, globalStep);
        playing = false;
        playingFlag.store (false, std::memory_order_relaxed);
        currentStep.store (-1, std::memory_order_relaxed);
        finishedCount.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    const int patternStep = static_cast<int> (globalStep % numSteps);
    const auto& step = patterns[readIndex].steps[patternStep];

    // Which of this step's slots are already sounding (and so just continue)?
    int matchedActive[SequencerPattern::maxSlots];
    bool carried[SequencerPattern::maxSlots] = {};

    for (int i = 0; i < step.numSlots; ++i)
    {
        matchedActive[i] = -1;
        const auto& slot = step.slots[i];

        if (slot.strike)
            continue;

        for (int j = 0; j < SequencerPattern::maxSlots; ++j)
        {
            if (active[j].used && ! carried[j] && active[j].slot.sameVoicing (slot))
            {
                matchedActive[i] = j;
                carried[j] = true;
                break;
            }
        }
    }

    // 1. Release what is no longer part of the pattern (note-offs go out before any note-on).
    for (int j = 0; j < SequencerPattern::maxSlots; ++j)
    {
        auto& a = active[j];
        if (! a.used || carried[j])
            continue;

        if (a.slot.arpSteps > 0)
        {
            if (a.soundingNote >= 0)
                push (Event::Type::noteOff, offset, a.soundingNote, 0.0f, false, 0, 0);
        }
        else
        {
            for (int i = 0; i < a.slot.numNotes; ++i)
                push (Event::Type::noteOff, offset, a.slot.notes[i], 0.0f, false, 0, 0);
        }

        a = ActiveSlot();
    }

    // 2. Slots that continue: sustained chords stay as they are, arpeggios move on when their
    //    next note falls on this step.
    for (int i = 0; i < step.numSlots; ++i)
    {
        const int j = matchedActive[i];
        if (j < 0)
            continue;

        auto& a = active[j];
        a.slot = step.slots[i];

        if (a.slot.arpSteps > 0 && a.slot.numNotes > 0
            && (globalStep - a.strikeStep) % a.slot.arpSteps == 0)
        {
            if (a.soundingNote >= 0)
                push (Event::Type::noteOff, offset, a.soundingNote, 0.0f, false, 0, 0);

            a.arpIndex = (a.arpIndex + 1) % a.slot.numNotes;
            a.soundingNote = a.slot.notes[a.arpIndex];
            push (Event::Type::noteOn, offset, a.soundingNote, a.slot.velocity, false, 0, 0);
        }
    }

    // 3. New strikes.
    for (int i = 0; i < step.numSlots; ++i)
    {
        if (matchedActive[i] >= 0)
            continue;

        const auto& slot = step.slots[i];

        for (int j = 0; j < SequencerPattern::maxSlots; ++j)
        {
            auto& a = active[j];
            if (a.used)
                continue;

            a.used = true;
            a.slot = slot;
            a.strikeStep = globalStep;
            a.arpIndex = 0;
            a.soundingNote = -1;

            if (slot.arpSteps > 0)
            {
                if (slot.numNotes > 0)
                {
                    a.soundingNote = slot.notes[0];
                    push (Event::Type::noteOn, offset, a.soundingNote, slot.velocity, false, 0, 0);
                }
            }
            else
            {
                for (int n = 0; n < slot.numNotes; ++n)
                    push (Event::Type::noteOn, offset, slot.notes[n], slot.velocity, false, 0, 0);
            }

            break;
        }
    }

    if (clickEnabled.load (std::memory_order_relaxed))
        push (Event::Type::click, offset, 0, 0.0f, (patternStep % 4) == 0, patternStep, globalStep);

    push (Event::Type::stepStarted, offset, 0, 0.0f, false, patternStep, globalStep);
    currentStep.store (patternStep, std::memory_order_relaxed);
}

//==============================================================================
int StepSequencer::process (int numSamples) noexcept
{
    // Worst case for one step: every slot released and struck with a full chord, plus a click
    // and the step marker.
    constexpr int worstCaseEventsPerStep = 2 * SequencerPattern::maxSlots * SequencerSlot::maxNotes + 2;

    numEvents = 0;
    pickUpPattern();

    if (! playing)
    {
        samplePos += numSamples;
        return numSamples;
    }

    applyTempoChange();

    const std::int64_t chunkStart = samplePos;
    const std::int64_t chunkEnd = chunkStart + numSamples;
    int consumed = numSamples;

    for (;;)
    {
        const auto position = static_cast<std::int64_t> (std::llround (boundaryOf (nextStep)));
        if (position >= chunkEnd)
            break;

        if (numEvents + worstCaseEventsPerStep > maxEvents)
        {
            consumed = static_cast<int> (juce::jlimit<std::int64_t> (0, numSamples, position - chunkStart));
            break;
        }

        const int offset = static_cast<int> (juce::jmax<std::int64_t> (0, position - chunkStart));
        fireStep (nextStep, offset);

        if (! playing)
            break;

        ++nextStep;
    }

    samplePos += consumed;
    return consumed;
}

} // namespace ChordFoundry
