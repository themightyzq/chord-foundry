#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>

#include "ArpeggiatorEngine.h"

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
    The headless rendering engine: a juce::Synthesiser plus a tiny click-track
    generator, mixed down through a polyphony-aware gain stage and a soft
    limiter. This class never touches an audio device - it is a plain
    juce::AudioSource that can be prepared and rendered directly into an
    AudioBuffer, which is what makes it possible to unit test without opening
    real audio hardware. ChordSynthesizer (below) is the only class that wires
    it up to a real device.
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
    // Note-level control. Safe to call from the message thread; juce::Synthesiser
    // guards its voice list with an internal lock shared with renderNextBlock().
    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber);
    void allNotesOff();

    // Triggers a short click-track burst (~2 ms). accent selects a higher,
    // louder blip for downbeats. Safe to call from the message thread; the
    // request is picked up by the audio thread via an atomic counter.
    void playClick (bool accent);

    void setMasterGain (float newGain);

    static constexpr int numVoices = 16;

private:
    void renderClick (const juce::AudioSourceChannelInfo& bufferToFill);
    void applyGainAndLimiter (const juce::AudioSourceChannelInfo& bufferToFill);

    juce::Synthesiser synth;
    std::atomic<float> masterGain { 0.8f };
    double currentSampleRate = 44100.0;

    // Click-track state. The atomics are the only fields the message thread
    // touches; everything else below them is audio-thread-only.
    std::atomic<int> clickRequestCounter { 0 };
    std::atomic<bool> clickRequestAccent { false };
    int lastHandledClickCounter = 0;
    int clickLengthSamples = 96; // re-derived from the sample rate in prepareToPlay
    int clickSamplesRemaining = 0;
    double clickPhase = 0.0;
    bool clickIsAccent = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordSynthAudioSource)
};

//==============================================================================
/**
    ChordSynthesizer - the audio engine used by MainComponent. Owns the
    default audio device and drives a ChordSynthAudioSource through it.

    Frequencies passed to playChord(const std::vector<float>&) are mapped to
    the nearest MIDI note (no fractional pitch-bend/offset support); use
    playChord(const std::vector<int>&, velocity) directly when exact MIDI
    notes are already known.
*/
class ChordSynthesizer
{
public:
    ChordSynthesizer();
    ~ChordSynthesizer();

    // Opens the default audio device (initialiseWithDefaultDevices(0, 2), 0 in /
    // 2 out) the first time it's called, then attaches the audio callback.
    // stop() detaches the callback but leaves the device open, so start() can
    // be called again cheaply.
    void start();
    void stop();
    bool isRunning() const { return running; }

    // Plays a chord as sustained notes (replaces whatever chord is currently
    // sounding). Frequencies are converted to the nearest MIDI note.
    void playChord (const std::vector<float>& frequencies);
    void playChord (const std::vector<int>& midiNotes, float velocity = 0.8f);

    // Plays `frequencies` as an arpeggio: notes are reordered by arpMode
    // (see ArpeggiatorEngine) and stepped through one at a time, each held
    // for the note length implied by arpLength at tempoBpm, looping until
    // stopAllNotes() or another play call replaces it.
    void playArpeggiatedChord (const std::vector<float>& frequencies,
                                const juce::String& arpMode,
                                const juce::String& arpLength,
                                float tempoBpm,
                                float velocity = 0.8f);

    void stopAllNotes();

    // Short click-track burst; accent selects a louder/higher blip for downbeats.
    void playClick (bool accent);

    void setMasterGain (float newGain);

    // Maps a frequency in Hz to the nearest MIDI note number (0-127, clamped).
    static int frequencyToNearestMidiNote (float frequencyHz);

private:
    class ArpeggiatorTimer;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    ChordSynthAudioSource audioSource;
    ArpeggiatorEngine arpeggiatorEngine;
    std::unique_ptr<ArpeggiatorTimer> arpeggiatorTimer;

    bool deviceInitialised = false;
    bool running = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordSynthesizer)
};

} // namespace ChordFoundry
