#include "ChordSynthesizer.h"

#include <cmath>

namespace ChordFoundry {

//==============================================================================
// ChordSynthVoice
//==============================================================================
ChordSynthVoice::ChordSynthVoice()
{
    adsr.setParameters(adsrParams);
}

bool ChordSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<ChordSynthSound*>(sound) != nullptr;
}

void ChordSynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentVelocity = velocity;
    osc.reset();
    osc.setFrequency(static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber)), true);
    adsr.setParameters(adsrParams);
    adsr.noteOn();
}

void ChordSynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void ChordSynthVoice::pitchWheelMoved(int) {}
void ChordSynthVoice::controllerMoved(int, int) {}

void ChordSynthVoice::setCurrentPlaybackSampleRate(double newRate)
{
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);

    if (newRate > 0.0)
    {
        juce::dsp::ProcessSpec spec { newRate, 512, 1 };
        osc.prepare(spec);
        adsr.setSampleRate(newRate);
    }
}

void ChordSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!adsr.isActive())
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float oscSample = osc.processSample(0.0f) * harmonicNormalisation;
        const float env = adsr.getNextSample();
        const float sample = oscSample * env * currentVelocity * voiceHeadroom;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + i, sample);

        if (!adsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

//==============================================================================
// ChordSynthAudioSource
//==============================================================================
ChordSynthAudioSource::ChordSynthAudioSource()
{
    synth.clearVoices();
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new ChordSynthVoice());

    synth.clearSounds();
    synth.addSound(new ChordSynthSound());
    synth.setNoteStealingEnabled(true);
}

void ChordSynthAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);

    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    synth.setCurrentPlaybackSampleRate(currentSampleRate);

    clickLengthSamples = juce::jmax(1, static_cast<int>(std::round(0.002 * currentSampleRate)));
    clickSamplesRemaining = 0;
    clickPhase = 0.0;
}

void ChordSynthAudioSource::releaseResources()
{
    clickSamplesRemaining = 0;
}

void ChordSynthAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    if (bufferToFill.buffer == nullptr)
        return;

    const juce::MidiBuffer noMidi;
    synth.renderNextBlock(*bufferToFill.buffer, noMidi, bufferToFill.startSample, bufferToFill.numSamples);

    renderClick(bufferToFill);
    applyGainAndLimiter(bufferToFill);
}

void ChordSynthAudioSource::renderClick(const juce::AudioSourceChannelInfo& bufferToFill)
{
    const int requested = clickRequestCounter.load(std::memory_order_acquire);
    if (requested != lastHandledClickCounter)
    {
        lastHandledClickCounter = requested;
        clickSamplesRemaining = clickLengthSamples;
        clickPhase = 0.0;
        clickIsAccent = clickRequestAccent.load(std::memory_order_acquire);
    }

    if (clickSamplesRemaining <= 0)
        return;

    const double frequencyHz = clickIsAccent ? 2600.0 : 1800.0;
    const double phaseIncrement = juce::MathConstants<double>::twoPi * frequencyHz / currentSampleRate;
    const float amplitude = clickIsAccent ? 0.5f : 0.35f;

    const int samplesToRender = juce::jmin(clickSamplesRemaining, bufferToFill.numSamples);

    for (int i = 0; i < samplesToRender; ++i)
    {
        // Linear fade-out over the burst so it doesn't click (pun intended)
        // at the end of its own envelope.
        const float envelope = static_cast<float>(clickSamplesRemaining) / static_cast<float>(clickLengthSamples);
        const float sample = amplitude * envelope * static_cast<float>(std::sin(clickPhase));
        clickPhase += phaseIncrement;

        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
            bufferToFill.buffer->addSample(ch, bufferToFill.startSample + i, sample);

        --clickSamplesRemaining;
    }
}

void ChordSynthAudioSource::applyGainAndLimiter(const juce::AudioSourceChannelInfo& bufferToFill)
{
    int activeVoices = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = synth.getVoice(i))
            if (voice->isVoiceActive())
                ++activeVoices;

    // 1/sqrt(N) mixer-stage scaling keeps a full chord from summing past unity
    // as voice count grows; the tanh below is a hard safety net on top of
    // that, so peak output stays under 0 dBFS regardless of chord size or
    // velocity.
    const float polyphonyGain = 1.0f / std::sqrt(static_cast<float>(juce::jmax(1, activeVoices)));
    const float gain = polyphonyGain * masterGain.load(std::memory_order_relaxed);

    for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
    {
        auto* data = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
            data[i] = std::tanh(data[i] * gain);
    }
}

void ChordSynthAudioSource::noteOn(int midiNoteNumber, float velocity)
{
    synth.noteOn(1, midiNoteNumber, velocity);
}

void ChordSynthAudioSource::noteOff(int midiNoteNumber)
{
    synth.noteOff(1, midiNoteNumber, 1.0f, true);
}

void ChordSynthAudioSource::allNotesOff()
{
    synth.allNotesOff(0, true);
}

void ChordSynthAudioSource::playClick(bool accent)
{
    clickRequestAccent.store(accent, std::memory_order_release);
    clickRequestCounter.fetch_add(1, std::memory_order_release);
}

void ChordSynthAudioSource::setMasterGain(float newGain)
{
    masterGain.store(juce::jlimit(0.0f, 1.0f, newGain), std::memory_order_relaxed);
}

//==============================================================================
// ChordSynthesizer::ArpeggiatorTimer
//==============================================================================
// Steps through a pre-computed note sequence on the message thread, one note
// per timer tick, driving the audio source's noteOn/noteOff. Loops until
// stop() is called (from stopAllNotes()) or begin() is called again with a
// new sequence.
class ChordSynthesizer::ArpeggiatorTimer : public juce::Timer
{
public:
    explicit ArpeggiatorTimer(ChordSynthAudioSource& sourceToUse) : audioSource(sourceToUse) {}

    ~ArpeggiatorTimer() override { stop(); }

    void begin(std::vector<float> sequenceToPlay, double secondsPerNote, float velocityToUse)
    {
        stopTimer();
        turnOffCurrentNote();

        sequence = std::move(sequenceToPlay);
        velocity = velocityToUse;
        index = 0;

        if (sequence.empty())
            return;

        playNextNote();

        const int intervalMs = juce::jmax(1, static_cast<int>(std::round(secondsPerNote * 1000.0)));
        startTimer(intervalMs);
    }

    void stop()
    {
        stopTimer();
        turnOffCurrentNote();
        sequence.clear();
    }

    void timerCallback() override
    {
        playNextNote();
    }

private:
    void playNextNote()
    {
        turnOffCurrentNote();

        if (sequence.empty())
        {
            stopTimer();
            return;
        }

        const float frequency = sequence[static_cast<size_t>(index)];
        currentMidiNote = ChordSynthesizer::frequencyToNearestMidiNote(frequency);
        audioSource.noteOn(currentMidiNote, velocity);

        index = (index + 1) % static_cast<int>(sequence.size());
    }

    void turnOffCurrentNote()
    {
        if (currentMidiNote >= 0)
        {
            audioSource.noteOff(currentMidiNote);
            currentMidiNote = -1;
        }
    }

    ChordSynthAudioSource& audioSource;
    std::vector<float> sequence;
    float velocity = 0.8f;
    int index = 0;
    int currentMidiNote = -1;
};

//==============================================================================
// ChordSynthesizer
//==============================================================================
ChordSynthesizer::ChordSynthesizer()
{
    arpeggiatorTimer = std::make_unique<ArpeggiatorTimer>(audioSource);
}

ChordSynthesizer::~ChordSynthesizer()
{
    arpeggiatorTimer->stop();
    stop();
}

void ChordSynthesizer::start()
{
    if (running)
        return;

    if (!deviceInitialised)
    {
        const auto error = deviceManager.initialiseWithDefaultDevices(0, 2);
        if (error.isNotEmpty())
            juce::Logger::writeToLog("ChordSynthesizer::start: could not open the default audio output: " + error);
        deviceInitialised = true; // stays silent without a device; the log line says why
    }

    audioSourcePlayer.setSource(&audioSource);
    deviceManager.addAudioCallback(&audioSourcePlayer);
    running = true;
}

void ChordSynthesizer::stop()
{
    if (!running)
        return;

    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    running = false;
}

void ChordSynthesizer::playChord(const std::vector<float>& frequencies)
{
    std::vector<int> midiNotes;
    midiNotes.reserve(frequencies.size());
    for (const auto freq : frequencies)
        midiNotes.push_back(frequencyToNearestMidiNote(freq));

    playChord(midiNotes, 0.8f);
}

void ChordSynthesizer::playChord(const std::vector<int>& midiNotes, float velocity)
{
    arpeggiatorTimer->stop();
    audioSource.allNotesOff();

    for (const auto note : midiNotes)
        audioSource.noteOn(note, velocity);
}

void ChordSynthesizer::playArpeggiatedChord(const std::vector<float>& frequencies,
                                             const juce::String& arpMode,
                                             const juce::String& arpLength,
                                             float tempoBpm,
                                             float velocity)
{
    if (frequencies.empty())
        return;

    audioSource.allNotesOff();

    auto sequence = arpeggiatorEngine.getArpeggioSequence(frequencies, arpMode);
    const double noteLengthSeconds = arpeggiatorEngine.getNoteLength(arpLength, tempoBpm);

    arpeggiatorTimer->begin(std::move(sequence), noteLengthSeconds, velocity);
}

void ChordSynthesizer::stopAllNotes()
{
    arpeggiatorTimer->stop();
    audioSource.allNotesOff();
}

void ChordSynthesizer::playClick(bool accent)
{
    audioSource.playClick(accent);
}

void ChordSynthesizer::setMasterGain(float newGain)
{
    audioSource.setMasterGain(newGain);
}

int ChordSynthesizer::frequencyToNearestMidiNote(float frequencyHz)
{
    if (frequencyHz <= 0.0f)
        return 0;

    const double midi = 69.0 + 12.0 * std::log2(static_cast<double>(frequencyHz) / 440.0);
    return juce::jlimit(0, 127, static_cast<int>(std::round(midi)));
}

} // namespace ChordFoundry
