#include "../Source/Audio/ChordSynthesizer.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>

using namespace ChordFoundry;

namespace {

constexpr double testSampleRate = 48000.0;
constexpr int testBlockSize = 512;

// Renders `numSamples` from `source` into `buffer` (which must already be
// sized numChannels x numSamples), block by block, exactly as an
// AudioIODeviceCallback would. No audio device is opened anywhere here -
// ChordSynthAudioSource is a plain juce::AudioSource.
void render(ChordSynthAudioSource& source, juce::AudioBuffer<float>& buffer, int numSamples)
{
    int done = 0;
    while (done < numSamples)
    {
        const int thisBlock = juce::jmin(testBlockSize, numSamples - done);
        juce::AudioSourceChannelInfo info(&buffer, done, thisBlock);
        source.getNextAudioBlock(info);
        done += thisBlock;
    }
}

float peakMagnitude(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, startSample, numSamples));
    return peak;
}

} // namespace

// Exercises the headless rendering engine (ChordSynthAudioSource) directly,
// with no audio device opened: chord playback stays under 0 dBFS for both a
// small and a full 10-note chord, stopAllNotes() lets the release tail decay
// away, and playClick() produces a short burst rather than a sustained tone.
class ChordSynthesizerTests : public juce::UnitTest
{
public:
    ChordSynthesizerTests() : juce::UnitTest("ChordSynthesizer", "Audio") {}

    void runTest() override
    {
        beginTest("3-note chord for 0.5s is non-silent and peaks below 1.0");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(testBlockSize, testSampleRate);

            const int numSamples = static_cast<int>(testSampleRate * 0.5);
            juce::AudioBuffer<float> buffer(2, numSamples);
            buffer.clear();

            source.noteOn(60, 0.8f); // C4
            source.noteOn(64, 0.8f); // E4
            source.noteOn(67, 0.8f); // G4

            render(source, buffer, numSamples);

            const float peak = peakMagnitude(buffer, 0, numSamples);
            expect(peak > 0.001f, "expected audible (non-silent) output from a sounding chord");
            expect(peak < 1.0f, "expected chord peak to stay below 1.0 (0 dBFS)");
        }

        beginTest("10-note chord at full velocity peaks below 1.0");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(testBlockSize, testSampleRate);

            const int numSamples = static_cast<int>(testSampleRate * 0.3);
            juce::AudioBuffer<float> buffer(2, numSamples);
            buffer.clear();

            const int notes[10] = { 48, 52, 55, 60, 64, 67, 72, 76, 79, 84 };
            for (int note : notes)
                source.noteOn(note, 1.0f);

            render(source, buffer, numSamples);

            const float peak = peakMagnitude(buffer, 0, numSamples);
            expect(peak > 0.001f, "expected audible output from a 10-note chord");
            expect(peak < 1.0f, "expected a 10-note, full-velocity chord to stay below 1.0 (0 dBFS)");
        }

        beginTest("stopAllNotes decays below -60 dBFS after one release time");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(testBlockSize, testSampleRate);

            // Let the chord reach its sustain level first.
            const int soundingSamples = static_cast<int>(testSampleRate * 0.2);
            juce::AudioBuffer<float> soundingBuffer(2, soundingSamples);
            soundingBuffer.clear();

            source.noteOn(60, 0.8f);
            source.noteOn(64, 0.8f);
            source.noteOn(67, 0.8f);
            render(source, soundingBuffer, soundingSamples);

            source.allNotesOff();

            // ADSR release is ~120ms; render well past that and check the
            // tail of the render, which should have fully decayed.
            const int releaseSamples = static_cast<int>(testSampleRate * 0.3);
            juce::AudioBuffer<float> releaseBuffer(2, releaseSamples);
            releaseBuffer.clear();
            render(source, releaseBuffer, releaseSamples);

            const int tailStart = static_cast<int>(testSampleRate * 0.2); // last 100ms of the release render
            const int tailLength = releaseSamples - tailStart;
            const float tailPeak = peakMagnitude(releaseBuffer, tailStart, tailLength);
            const float tailPeakDb = juce::Decibels::gainToDecibels(tailPeak, -120.0f);

            expect(tailPeakDb < -60.0f, "expected the release tail to decay below -60 dBFS");
        }

        beginTest("playClick produces a burst shorter than 20ms");
        {
            ChordSynthAudioSource source;
            source.prepareToPlay(testBlockSize, testSampleRate);

            const int numSamples = static_cast<int>(testSampleRate * 0.1); // 100ms, plenty of room
            juce::AudioBuffer<float> buffer(2, numSamples);
            buffer.clear();

            source.playClick(false);
            render(source, buffer, numSamples);

            // Find the last sample whose magnitude is still meaningfully
            // above silence; everything after it should be the burst's tail.
            const float* data = buffer.getReadPointer(0);
            int lastLoudSample = -1;
            for (int i = 0; i < numSamples; ++i)
                if (std::abs(data[i]) > 1.0e-4f)
                    lastLoudSample = i;

            expect(lastLoudSample >= 0, "expected playClick to produce some audible output");

            const double burstLengthMs = (lastLoudSample + 1) * 1000.0 / testSampleRate;
            expect(burstLengthMs < 20.0, "expected the click burst to be shorter than 20ms");
        }
    }
};

static ChordSynthesizerTests chordSynthesizerTestsInstance;
