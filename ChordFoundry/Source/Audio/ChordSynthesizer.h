#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "StepSequencer.h"

namespace ChordFoundry {

//==============================================================================
/** The one sound every ChordSynthVoice can play: accepts all notes and all
    MIDI channels, so any voice can be assigned to any incoming note. */
class ChordSynthSound : public juce::SynthesiserSound
{
public:
    ChordSynthSound() = default;

    bool appliesToNote (int) override        { return true; }
    bool appliesToChannel (int) override     { return true; }
};

//==============================================================================
/** A single polyphonic voice: a soft oscillator (fundamental plus a little
    2nd and 3rd harmonic, so chords sound warm rather than buzzy) shaped by an
    ADSR envelope. No allocation happens outside prepare/setCurrentPlaybackSampleRate. */
class ChordSynthVoice : public juce::SynthesiserVoice
{
public:
    ChordSynthVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    void setCurrentPlaybackSampleRate (double newRate) override;

private:
    juce::dsp::Oscillator<float> osc { [] (float x)
    {
        // Fundamental plus a touch of 2nd/3rd harmonic - a soft, pleasant
        // timbre for stacked chord tones rather than a bare sine or a buzzy
        // saw/square. Peak amplitude before normalisation is at most the sum
        // of the coefficients (1.225); see harmonicNormalisation below.
        return std::sin (x) + 0.15f * std::sin (2.0f * x) + 0.075f * std::sin (3.0f * x);
    } };

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams { 0.005f, 0.08f, 0.7f, 0.12f }; // attack, decay, sustain, release
    float currentVelocity = 0.0f;

    static constexpr float harmonicNormalisation = 1.0f / 1.225f;
    static constexpr float voiceHeadroom = 0.7f; // extra headroom so a full chord can't clip a single voice

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordSynthVoice)
};

//==============================================================================
/**
    The headless rendering engine: a juce::Synthesiser, a click-track generator and the
    sample-accurate StepSequencer, mixed down through a polyphony-aware gain stage and a soft
    limiter. This class never touches an audio device - it is a plain juce::AudioSource that
    can be prepared and rendered directly into an AudioBuffer, which is what makes it possible
    to unit test without opening real audio hardware. ChordSynthesizer (below) is the only
    class that wires it up to a real device.

    Threading: every public control method is for the message thread (one caller thread). They
    never touch the synthesiser; they post a command to a lock-free FIFO that the audio thread
    drains at the start of each block, so the audio callback never waits on the message thread.
*/
class ChordSynthAudioSource : public juce::AudioSource
{
public:
    ChordSynthAudioSource();
    ~ChordSynthAudioSource() override = default;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Note-level control (auditioning a chord outside the sequencer).
    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber);
    void allNotesOff();

    // Triggers a short click-track burst (~2 ms). accent selects a higher, louder blip for
    // downbeats.
    void playClick (bool accent);

    void setMasterGain (float newGain);
    float getMasterGain() const { return masterGain.load (std::memory_order_relaxed); }

    //==============================================================================
    // Step sequencer. Step timing is counted in samples inside the audio callback.
    void setSequencerPattern (const SequencerPattern& pattern)  { sequencer.setPattern (pattern); }
    void setTempo (float bpm)                                   { sequencer.setTempo (bpm); }
    void setLoop (bool shouldLoop)                              { sequencer.setLoop (shouldLoop); }
    void setSequencerClickEnabled (bool enabled)                { sequencer.setClickEnabled (enabled); }
    void startSequencer();
    void stopSequencer();

    int getSequencerStep() const                { return sequencer.getCurrentStep(); }
    bool isSequencerPlaying() const             { return sequencer.isPlaying(); }
    int getSequencerFinishedCount() const       { return sequencer.getFinishedCount(); }

    // Number of note-ons the audio thread has handed to the synthesiser (diagnostics/tests).
    int getNoteOnCount() const                  { return noteOnCount.load (std::memory_order_relaxed); }

    static constexpr int numVoices = 16;

private:
    struct Command
    {
        enum class Type : std::uint8_t { noteOn, noteOff, allNotesOff, click, startSequencer, stopSequencer };

        Type type = Type::allNotesOff;
        int note = 0;
        float velocity = 0.0f;
        bool accent = false;
    };

    static constexpr int commandFifoSize = 1024;

    void post (const Command& command);
    void drainCommands();
    void apply (const Command& command);
    void apply (const StepSequencer::Event& event);
    void renderSegment (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void startClick (bool accent);
    void renderClick (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void applyGainAndLimiter (const juce::AudioSourceChannelInfo& bufferToFill);

    juce::Synthesiser synth;
    StepSequencer sequencer;
    const juce::MidiBuffer noMidi;
    std::atomic<float> masterGain { 0.8f };
    std::atomic<int> noteOnCount { 0 };
    double currentSampleRate = 44100.0;

    juce::AbstractFifo commandFifo { commandFifoSize };
    std::vector<Command> commands = std::vector<Command> (static_cast<size_t> (commandFifoSize));

    // Click-track state (audio thread only).
    int clickLengthSamples = 96; // re-derived from the sample rate in prepareToPlay
    int clickSamplesRemaining = 0;
    double clickPhase = 0.0;
    bool clickIsAccent = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordSynthAudioSource)
};

//==============================================================================
/**
    ChordSynthesizer - the audio engine used by MainComponent. Owns the default audio
    device and drives a ChordSynthAudioSource through it.

    Frequencies passed to playChord(const std::vector<float>&) are mapped to the nearest MIDI
    note (no fractional pitch-bend/offset support); use playChord(const std::vector<int>&,
    velocity) directly when exact MIDI notes are already known.
*/
class ChordSynthesizer : private juce::ChangeListener
{
public:
    ChordSynthesizer();
    ~ChordSynthesizer() override;

    // Opens the default audio output device (0 in / 2 out) and attaches the audio callback.
    // Returns true when a usable output device is open. When it is not, getDeviceProblem()
    // says why in words a user can act on. Calling start() again retries a device that failed
    // to open. stop() detaches the callback but leaves the device open.
    bool start();
    void stop();
    bool isRunning() const { return running; }

    // True when an output device with at least one output channel is open.
    bool hasAudioDevice() const;

    // Empty when the output device is fine; otherwise a short explanation for the user.
    juce::String getDeviceProblem() const;

    // Builds the explanation from the error text AudioDeviceManager returned and the state of
    // the device afterwards. Separate and static so it can be tested without hardware.
    static juce::String describeDeviceProblem (const juce::String& initialiseError,
                                               bool deviceOpen, bool hasOutputChannels);

    // For the audio settings dialog.
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // Called on the message thread whenever the audio device configuration changes.
    std::function<void()> onDeviceStateChanged;

    // Plays a chord as sustained notes (replaces whatever chord is currently sounding).
    // Frequencies are converted to the nearest MIDI note.
    void playChord (const std::vector<float>& frequencies);
    void playChord (const std::vector<int>& midiNotes, float velocity = 0.8f);

    void stopAllNotes();

    // Short click-track burst; accent selects a louder/higher blip for downbeats.
    void playClick (bool accent);

    void setMasterGain (float newGain);

    // Sequenced playback: the pattern is counted out in samples on the audio thread.
    void setSequencerPattern (const SequencerPattern& pattern)  { audioSource.setSequencerPattern (pattern); }
    void setTempo (float bpm)                                   { audioSource.setTempo (bpm); }
    void setLoop (bool shouldLoop)                              { audioSource.setLoop (shouldLoop); }
    void setSequencerClickEnabled (bool enabled)                { audioSource.setSequencerClickEnabled (enabled); }
    void startSequencer()                                       { audioSource.startSequencer(); }
    void stopSequencer();
    int getSequencerStep() const                                { return audioSource.getSequencerStep(); }
    int getSequencerFinishedCount() const                       { return audioSource.getSequencerFinishedCount(); }

    // Maps a frequency in Hz to the nearest MIDI note number (0-127, clamped).
    static int frequencyToNearestMidiNote (float frequencyHz);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    ChordSynthAudioSource audioSource;

    juce::String lastInitialiseError;
    bool deviceInitialised = false;
    bool running = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordSynthesizer)
};

} // namespace ChordFoundry
