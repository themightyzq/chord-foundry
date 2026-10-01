#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <cstdint>

namespace ChordFoundry {

//==============================================================================
/** One chord (or arpeggio) sounding during one sequencer step. Plain data so a
    whole pattern can be copied to the audio thread without allocating. */
struct SequencerSlot
{
    static constexpr int maxNotes = 16;

    int numNotes = 0;
    std::uint8_t notes[maxNotes] {};   // MIDI notes; already in arpeggio order when arpSteps > 0
    int arpSteps = 0;                  // 0 = sustained chord, else one arp note every arpSteps steps
    bool strike = false;               // force a new strike even if the previous step held the same slot
    float velocity = 0.8f;

    /** True if `other` would sound exactly the same (notes, arp rate, velocity). Whether the
        slot is marked as a new strike is deliberately not part of the comparison. */
    bool sameVoicing (const SequencerSlot& other) const noexcept;
};

/** The 32-step grid, flattened for the audio thread. */
struct SequencerPattern
{
    static constexpr int numSteps = 32;
    static constexpr int maxSlots = 8;

    struct Step
    {
        int numSlots = 0;
        SequencerSlot slots[maxSlots];
    };

    Step steps[numSteps];

    void clear() noexcept;
};

//==============================================================================
/**
    Sample-accurate step scheduler. One step is a 16th note. The scheduler counts audio
    samples, never wall-clock time: call process() once per audio block and it returns the
    events that fall inside that block, each with its sample offset.

    Step n starts at  base + (n - baseStep) * stepLength  samples (a double, rounded once per
    event), so a fractional step length never accumulates error and the result does not
    depend on the block size.

    Sustained chords: when step n holds a slot with the same voicing as one that is already
    sounding, the notes are not re-struck. A slot is struck only when it is new, its voicing
    changed, or it is flagged as a new strike. Arpeggios keep their position across steps for
    the same reason.

    Threading: the setters (setTempo, setLoop, setClickEnabled, setPattern) are for the
    message thread (one writer). startNow/stopNow/process are for the audio thread only.
    Nothing here allocates, locks or logs.
*/
class StepSequencer
{
public:
    struct Event
    {
        enum class Type : std::uint8_t { noteOn, noteOff, click, stepStarted, finished };

        Type type = Type::noteOn;
        int sampleOffset = 0;   // relative to the start of the chunk passed to process()
        int note = 0;
        float velocity = 0.0f;
        bool accent = false;
        int step = 0;           // pattern step (0-31) for stepStarted
        std::int64_t absoluteStep = 0;
    };

    static constexpr int maxEvents = 1024;

    StepSequencer();

    //==============================================================================
    // Message thread
    void setTempo (float bpm) noexcept;
    void setLoop (bool shouldLoop) noexcept          { loop.store (shouldLoop, std::memory_order_relaxed); }
    void setClickEnabled (bool enabled) noexcept     { clickEnabled.store (enabled, std::memory_order_relaxed); }
    void setPattern (const SequencerPattern& newPattern);

    //==============================================================================
    // Audio thread
    void prepare (double newSampleRate) noexcept;
    void startNow() noexcept;
    void stopNow() noexcept;

    /** Generates the events for the next `numSamples` samples. Returns how many samples
        were consumed: normally numSamples, fewer only if the event buffer filled up, in
        which case the caller applies the events and calls again for the remainder. */
    int process (int numSamples) noexcept;

    const Event* getEvents() const noexcept          { return events; }
    int getNumEvents() const noexcept                { return numEvents; }

    //==============================================================================
    // Any thread
    int getCurrentStep() const noexcept              { return currentStep.load (std::memory_order_relaxed); }
    bool isPlaying() const noexcept                  { return playingFlag.load (std::memory_order_relaxed); }

    /** Incremented each time playback ends by itself (non-looping pattern reached its end). */
    int getFinishedCount() const noexcept            { return finishedCount.load (std::memory_order_relaxed); }

    /** Samples per step at `bpm` and `rate`: 60 / bpm / 4 * rate. */
    static double getStepLengthSamples (double bpm, double rate) noexcept;

    /** The tempo range the app allows. */
    static float clampTempo (float bpm) noexcept     { return juce::jlimit (40.0f, 240.0f, bpm); }

private:
    struct ActiveSlot
    {
        bool used = false;
        SequencerSlot slot;
        std::int64_t strikeStep = 0;   // global step at which this slot was struck
        int arpIndex = 0;
        int soundingNote = -1;
    };

    double boundaryOf (std::int64_t globalStep) const noexcept;
    void pickUpPattern() noexcept;
    void applyTempoChange() noexcept;
    void fireStep (std::int64_t globalStep, int offset) noexcept;
    void releaseAll (int offset) noexcept;
    void push (Event::Type type, int offset, int note, float velocity, bool accent, int step, std::int64_t absoluteStep) noexcept;

    // Shared with the message thread
    std::atomic<float> tempo { 120.0f };
    std::atomic<bool> loop { false };
    std::atomic<bool> clickEnabled { false };
    std::atomic<int> currentStep { -1 };
    std::atomic<bool> playingFlag { false };
    std::atomic<int> finishedCount { 0 };

    // Pattern hand-off: a triple buffer. The writer fills `patterns[writeIndex]`, then swaps it
    // with `middle` (flagging it dirty); the reader swaps its slot in when it sees the flag.
    static constexpr int dirtyBit = 4;
    SequencerPattern patterns[3];
    int writeIndex = 0;
    int readIndex = 1;
    std::atomic<int> middle { 2 };

    // Audio thread only
    double sampleRate = 44100.0;
    bool playing = false;
    std::int64_t samplePos = 0;      // absolute sample count at the start of the next chunk
    std::int64_t nextStep = 0;       // global step counter of the next step to fire
    double baseSample = 0.0;
    std::int64_t baseStep = 0;
    double stepLength = 6000.0;
    float currentBpm = 120.0f;
    ActiveSlot active[SequencerPattern::maxSlots];

    Event events[maxEvents];
    int numEvents = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencer)
};

} // namespace ChordFoundry
