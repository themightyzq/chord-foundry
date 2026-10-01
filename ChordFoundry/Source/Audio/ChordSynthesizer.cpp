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
    sequencer.prepare(currentSampleRate);

    clickLengthSamples = juce::jmax(1, static_cast<int>(std::round(0.002 * currentSampleRate)));
    clickSamplesRemaining = 0;
    clickPhase = 0.0;
}

void ChordSynthAudioSource::releaseResources()
{
    clickSamplesRemaining = 0;
}

void ChordSynthAudioSource::post(const Command& command)
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    commandFifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 + size2 < 1)
        return; // full: nothing is draining it (no device); dropping is safe

    commands[static_cast<size_t>(size1 > 0 ? start1 : start2)] = command;
    commandFifo.finishedWrite(1);
}

void ChordSynthAudioSource::drainCommands()
{
    const int ready = commandFifo.getNumReady();
    if (ready == 0)
        return;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    commandFifo.prepareToRead(ready, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        apply(commands[static_cast<size_t>(start1 + i)]);
    for (int i = 0; i < size2; ++i)
        apply(commands[static_cast<size_t>(start2 + i)]);

    commandFifo.finishedRead(size1 + size2);
}

void ChordSynthAudioSource::apply(const Command& command)
{
    switch (command.type)
    {
        case Command::Type::noteOn:
            synth.noteOn(1, command.note, command.velocity);
            noteOnCount.fetch_add(1, std::memory_order_relaxed);
            break;
        case Command::Type::noteOff:
            synth.noteOff(1, command.note, 1.0f, true);
            break;
        case Command::Type::allNotesOff:
            synth.allNotesOff(0, true);
            break;
        case Command::Type::click:
            startClick(command.accent);
            break;
        case Command::Type::startSequencer:
            sequencer.startNow();
            break;
        case Command::Type::stopSequencer:
            sequencer.stopNow();
            synth.allNotesOff(0, true);
            break;
    }
}

void ChordSynthAudioSource::apply(const StepSequencer::Event& event)
{
    switch (event.type)
    {
        case StepSequencer::Event::Type::noteOn:
            synth.noteOn(1, event.note, event.velocity);
            noteOnCount.fetch_add(1, std::memory_order_relaxed);
            break;
        case StepSequencer::Event::Type::noteOff:
            synth.noteOff(1, event.note, 1.0f, true);
            break;
        case StepSequencer::Event::Type::click:
            startClick(event.accent);
            break;
        case StepSequencer::Event::Type::stepStarted:
        case StepSequencer::Event::Type::finished:
            break;
    }
}

void ChordSynthAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::ScopedNoDenormals noDenormals;

    bufferToFill.clearActiveBufferRegion();

    if (bufferToFill.buffer == nullptr)
        return;

    drainCommands();

    // The sequencer works out where steps fall inside this block, in samples. Render up to each
    // event, apply it, and carry on, so notes start on the exact sample.
    int done = 0;
    while (done < bufferToFill.numSamples)
    {
        const int consumed = sequencer.process(bufferToFill.numSamples - done);
        const auto* events = sequencer.getEvents();
        const int numEvents = sequencer.getNumEvents();

        int cursor = 0;
        for (int i = 0; i < numEvents; ++i)
        {
            const int offset = juce::jlimit(cursor, juce::jmax(cursor, consumed), events[i].sampleOffset);
            renderSegment(*bufferToFill.buffer, bufferToFill.startSample + done + cursor, offset - cursor);
            cursor = offset;
            apply(events[i]);
        }

        renderSegment(*bufferToFill.buffer, bufferToFill.startSample + done + cursor, consumed - cursor);
        done += consumed;
    }

    applyGainAndLimiter(bufferToFill);
}

void ChordSynthAudioSource::renderSegment(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    synth.renderNextBlock(buffer, noMidi, startSample, numSamples);
    renderClick(buffer, startSample, numSamples);
}

void ChordSynthAudioSource::startClick(bool accent)
{
    clickSamplesRemaining = clickLengthSamples;
    clickPhase = 0.0;
    clickIsAccent = accent;
}

void ChordSynthAudioSource::renderClick(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (clickSamplesRemaining <= 0)
        return;

    const double frequencyHz = clickIsAccent ? 2600.0 : 1800.0;
    const double phaseIncrement = juce::MathConstants<double>::twoPi * frequencyHz / currentSampleRate;
    const float amplitude = clickIsAccent ? 0.5f : 0.35f;

    const int samplesToRender = juce::jmin(clickSamplesRemaining, numSamples);

    for (int i = 0; i < samplesToRender; ++i)
    {
        // Linear fade-out over the burst so it doesn't click (pun intended)
        // at the end of its own envelope.
        const float envelope = static_cast<float>(clickSamplesRemaining) / static_cast<float>(clickLengthSamples);
        const float sample = amplitude * envelope * static_cast<float>(std::sin(clickPhase));
        clickPhase += phaseIncrement;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, startSample + i, sample);

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
    Command c;
    c.type = Command::Type::noteOn;
    c.note = midiNoteNumber;
    c.velocity = velocity;
    post(c);
}

void ChordSynthAudioSource::noteOff(int midiNoteNumber)
{
    Command c;
    c.type = Command::Type::noteOff;
    c.note = midiNoteNumber;
    post(c);
}

void ChordSynthAudioSource::allNotesOff()
{
    Command c;
    c.type = Command::Type::allNotesOff;
    post(c);
}

void ChordSynthAudioSource::playClick(bool accent)
{
    Command c;
    c.type = Command::Type::click;
    c.accent = accent;
    post(c);
}

void ChordSynthAudioSource::startSequencer()
{
    Command c;
    c.type = Command::Type::startSequencer;
    post(c);
}

void ChordSynthAudioSource::stopSequencer()
{
    Command c;
    c.type = Command::Type::stopSequencer;
    post(c);
}

void ChordSynthAudioSource::setMasterGain(float newGain)
{
    masterGain.store(juce::jlimit(0.0f, 1.0f, newGain), std::memory_order_relaxed);
}

//==============================================================================
// ChordSynthesizer
//==============================================================================
ChordSynthesizer::ChordSynthesizer()
{
    deviceManager.addChangeListener(this);
}

ChordSynthesizer::~ChordSynthesizer()
{
    deviceManager.removeChangeListener(this);
    stop();
}

bool ChordSynthesizer::start()
{
    if (!deviceInitialised || !hasAudioDevice())
    {
        // First call, or a retry after the device failed to open or went away.
        lastInitialiseError = deviceManager.initialiseWithDefaultDevices(0, 2);
        deviceInitialised = true;

        if (lastInitialiseError.isNotEmpty())
            juce::Logger::writeToLog("ChordSynthesizer::start: could not open the default audio output: " + lastInitialiseError);
    }

    if (!running)
    {
        audioSourcePlayer.setSource(&audioSource);
        deviceManager.addAudioCallback(&audioSourcePlayer);
        running = true;
    }

    return hasAudioDevice();
}

void ChordSynthesizer::stop()
{
    if (!running)
        return;

    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    running = false;
}

bool ChordSynthesizer::hasAudioDevice() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return !device->getActiveOutputChannels().isZero();

    return false;
}

juce::String ChordSynthesizer::getDeviceProblem() const
{
    auto* device = deviceManager.getCurrentAudioDevice();
    const bool hasOutputs = device != nullptr && !device->getActiveOutputChannels().isZero();
    return describeDeviceProblem(lastInitialiseError, device != nullptr, hasOutputs);
}

juce::String ChordSynthesizer::describeDeviceProblem(const juce::String& initialiseError,
                                                      bool deviceOpen, bool hasOutputChannels)
{
    if (deviceOpen && hasOutputChannels)
        return {};

    juce::String text = "Chord Foundry could not open an audio output device, so you will hear nothing.";

    if (deviceOpen)
        text += " The selected device has no output channels.";
    else if (initialiseError.isNotEmpty())
        text += " The system reported: " + initialiseError.trim();
    else
        text += " No output device is available.";

    text += "\n\nChoose another output in Audio Settings, or check that your speakers or interface are connected and not in use by another app.";
    return text;
}

void ChordSynthesizer::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (onDeviceStateChanged)
        onDeviceStateChanged();
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
    audioSource.allNotesOff();

    for (const auto note : midiNotes)
        audioSource.noteOn(note, velocity);
}

void ChordSynthesizer::stopSequencer()
{
    audioSource.stopSequencer();
}

void ChordSynthesizer::stopAllNotes()
{
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
