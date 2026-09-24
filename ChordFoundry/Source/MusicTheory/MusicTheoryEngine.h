#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include "../Data/ChordProgression.h"
#include <vector>
#include <unordered_map>

namespace ChordFoundry {

class MusicTheoryEngine {
public:
    // Core music theory data - matches Python implementation exactly
    static const std::vector<std::vector<int>> MODES;
    static const std::unordered_map<juce::String, int> KEY_TO_SEMITONE;
    static const std::unordered_map<juce::String, int> ROMAN_TO_DEGREE;
    static const std::vector<juce::String> MODE_NAMES;
    static const std::vector<juce::String> EXTENSION_NAMES;
    static const std::vector<juce::String> INVERSION_NAMES;
    static const std::vector<juce::String> VOICING_NAMES;
    static const std::vector<juce::String> ARP_MODE_NAMES;
    static const std::vector<juce::String> ARP_LENGTH_NAMES;
    static const std::vector<juce::String> SPREAD_TYPE_NAMES;
    
    // Main chord generation functions
    static std::vector<float> getChordFrequencies(
        const juce::String& roman,
        const juce::String& key,
        const juce::String& mode,
        const ChordData& modifiers
    );
    
    static std::vector<int> getChordMidiNotes(
        const juce::String& roman,
        const juce::String& key,
        const juce::String& mode,
        const ChordData& modifiers
    );
    
    // Core conversion functions
    static float midiToFrequency(int midiNote);
    static int frequencyToMidi(float frequency);
    
    // Scale and chord building
    static std::vector<int> getScale(const juce::String& key, const juce::String& mode);
    static std::vector<int> buildTriad(int rootNote, const std::vector<int>& scale, int degree);
    static std::vector<int> applyExtension(
        const std::vector<int>& triad,
        const juce::String& extension,
        const std::vector<int>& scale,
        int degree
    );
    static std::vector<int> applyInversion(
        const std::vector<int>& chord,
        const juce::String& inversion
    );
    static std::vector<int> applyVoicing(
        const std::vector<int>& chord,
        const juce::String& voicing,
        const CustomVoicingData& customData
    );
    
    // Custom voicing algorithms (matches Python spread types exactly)
    static std::vector<int> getCustomVoicing(
        const std::vector<int>& baseChord,
        const CustomVoicingData& voicingData
    );
    
    // Validation functions
    static bool isValidKey(const juce::String& key);
    static bool isValidMode(const juce::String& mode);
    static bool isValidRoman(const juce::String& roman);
    static bool isValidExtension(const juce::String& extension);
    static bool isValidInversion(const juce::String& inversion);
    static bool isValidVoicing(const juce::String& voicing);
    
    // Utility functions
    static juce::String getChordDisplayName(const ChordData& chord);
    static juce::Colour getChordColor(const juce::String& roman);
    static int getDegreeFromRoman(const juce::String& roman);
    static bool isChordMajor(const juce::String& roman);
    static bool isChordMinor(const juce::String& roman);
    static bool isChordDiminished(const juce::String& roman);
    
    // Constants matching Python implementation
    static constexpr float A4_FREQUENCY = 440.0f;
    static constexpr int A4_MIDI_NOTE = 69;
    static constexpr int OCTAVE_SIZE = 12;
    static constexpr int DEFAULT_OCTAVE = 4;
    static constexpr int BASE_MIDI_NOTE = 60; // C4
    
private:
    // Helper functions for chord building
    static int getKeyRootNote(const juce::String& key);
    static std::vector<int> getModeIntervals(const juce::String& mode);
    static int findDegreeInScale(const std::vector<int>& scale, int degree);
    static std::vector<int> stackThirds(const std::vector<int>& scale, int rootIndex, int numNotes);
    
    // Custom voicing spread algorithms
    static std::vector<int> applyStackedThirds(const std::vector<int>& chord, const CustomVoicingData& data);
    static std::vector<int> applyLayeredVoicing(const std::vector<int>& chord, const CustomVoicingData& data);
    static std::vector<int> applyClosedAboveBass(const std::vector<int>& chord, const CustomVoicingData& data);
    static std::vector<int> applyNoteVariability(const std::vector<int>& chord, const CustomVoicingData& data);
    static std::vector<int> applyRootFifthSpread(const std::vector<int>& chord, const CustomVoicingData& data);
    static std::vector<int> applyRootGuideTones(const std::vector<int>& chord, const CustomVoicingData& data);
    
    // Octave and range management
    static std::vector<int> adjustToOctave(const std::vector<int>& notes, int octave);
    static std::vector<int> ensureRange(const std::vector<int>& notes, int minNote, int maxNote);
    static std::vector<int> removeDuplicates(const std::vector<int>& notes);
    static std::vector<int> sortNotes(const std::vector<int>& notes);
    
    MusicTheoryEngine() = delete; // Static class only
};

} // namespace ChordFoundry