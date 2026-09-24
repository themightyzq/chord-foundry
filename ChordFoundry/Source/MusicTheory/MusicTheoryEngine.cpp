#include "MusicTheoryEngine.h"
#include <algorithm>
#include <random>

namespace ChordFoundry {

// Static data initialization - exact matches to Python implementation
const std::vector<std::vector<int>> MusicTheoryEngine::MODES = {
    {0, 2, 4, 5, 7, 9, 11},   // Major (Ionian)
    {0, 2, 3, 5, 7, 9, 10},   // Dorian
    {0, 1, 3, 5, 7, 8, 10},   // Phrygian
    {0, 2, 4, 6, 7, 9, 11},   // Lydian
    {0, 2, 4, 5, 7, 9, 10},   // Mixolydian
    {0, 2, 3, 5, 7, 8, 10},   // Minor (Aeolian)
    {0, 1, 3, 5, 6, 8, 10},   // Locrian
    {0, 1, 4, 5, 7, 8, 11},   // Gypsy Minor
    {0, 2, 3, 5, 7, 8, 11},   // Harmonic Minor
    {0, 3, 5, 7, 10},         // Minor Pentatonic
    {0, 2, 4, 6, 8, 10},      // Whole Tone
    {0, 1, 2, 5, 7, 8, 9},    // Tonic 2nds
    {0, 2, 3, 4, 7, 9, 10},   // Tonic 3rds
    {0, 2, 4, 5, 6, 9, 11},   // Tonic 4ths
    {0, 2, 4, 5, 7, 9, 10}    // Tonic 6ths
};

const std::unordered_map<juce::String, int> MusicTheoryEngine::KEY_TO_SEMITONE = {
    {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3},
    {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"G", 7}, {"G#", 8},
    {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, {"B", 11}
};

const std::unordered_map<juce::String, int> MusicTheoryEngine::ROMAN_TO_DEGREE = {
    {"I", 0}, {"ii", 1}, {"iii", 2}, {"IV", 3}, {"V", 4}, {"vi", 5}, {"vii°", 6}
};

const std::vector<juce::String> MusicTheoryEngine::MODE_NAMES = {
    "Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Minor", "Locrian",
    "Gypsy Minor", "Harmonic Minor", "Minor Pentatonic", "Whole Tone",
    "Tonic 2nds", "Tonic 3rds", "Tonic 4ths", "Tonic 6ths"
};

const std::vector<juce::String> MusicTheoryEngine::EXTENSION_NAMES = {
    "None", "+6th", "+7th", "+9th", "sus2", "sus4"
};

const std::vector<juce::String> MusicTheoryEngine::INVERSION_NAMES = {
    "None", "Root", "1st", "2nd"
};

const std::vector<juce::String> MusicTheoryEngine::VOICING_NAMES = {
    "None", "Root", "Open", "Drop 2", "Custom"
};

const std::vector<juce::String> MusicTheoryEngine::ARP_MODE_NAMES = {
    "None", "Up", "Down", "Random", "Converge", "Diverge", "Ascending", "Descending"
};

const std::vector<juce::String> MusicTheoryEngine::ARP_LENGTH_NAMES = {
    "1/16", "1/8", "1/4", "1/2"
};

const std::vector<juce::String> MusicTheoryEngine::SPREAD_TYPE_NAMES = {
    "Stacked Thirds", "Layered Voicing", "Closed Above Bass", 
    "Note Variability", "Root + Fifth Spread", "Root + Guide Tones"
};

// Main chord generation functions
std::vector<float> MusicTheoryEngine::getChordFrequencies(
    const juce::String& roman,
    const juce::String& key,
    const juce::String& mode,
    const ChordData& modifiers)
{
    auto midiNotes = getChordMidiNotes(roman, key, mode, modifiers);
    std::vector<float> frequencies;
    
    for (int midiNote : midiNotes) {
        frequencies.push_back(midiToFrequency(midiNote));
    }
    
    return frequencies;
}

std::vector<int> MusicTheoryEngine::getChordMidiNotes(
    const juce::String& roman,
    const juce::String& key,
    const juce::String& mode,
    const ChordData& modifiers)
{
    // Validate inputs
    if (!isValidRoman(roman) || !isValidKey(key) || !isValidMode(mode)) {
        return {};
    }
    
    // Get scale
    auto scale = getScale(key, mode);
    if (scale.empty()) {
        return {};
    }
    
    // Get roman numeral degree
    int degree = getDegreeFromRoman(roman);
    if (degree < 0) {
        return {};
    }
    
    // Build basic triad
    auto chord = buildTriad(scale[0], scale, degree);
    
    // Apply extensions
    if (modifiers.hasExtension()) {
        chord = applyExtension(chord, modifiers.extension, scale, degree);
    }
    
    // Apply inversions
    if (modifiers.hasInversion()) {
        chord = applyInversion(chord, modifiers.inversion);
    }
    
    // Apply voicing
    if (modifiers.hasVoicing()) {
        chord = applyVoicing(chord, modifiers.voicing, modifiers.customVoicing);
    }
    
    // Sort and remove duplicates
    chord = removeDuplicates(chord);
    chord = sortNotes(chord);
    
    return chord;
}

// Core conversion functions
float MusicTheoryEngine::midiToFrequency(int midiNote)
{
    return A4_FREQUENCY * std::pow(2.0f, (midiNote - A4_MIDI_NOTE) / 12.0f);
}

int MusicTheoryEngine::frequencyToMidi(float frequency)
{
    return static_cast<int>(A4_MIDI_NOTE + 12.0f * std::log2(frequency / A4_FREQUENCY) + 0.5f);
}

// Scale and chord building
std::vector<int> MusicTheoryEngine::getScale(const juce::String& key, const juce::String& mode)
{
    int rootNote = getKeyRootNote(key);
    if (rootNote < 0) return {};
    
    auto intervals = getModeIntervals(mode);
    if (intervals.empty()) return {};
    
    std::vector<int> scale;
    for (int interval : intervals) {
        scale.push_back(rootNote + interval);
    }
    
    return scale;
}

std::vector<int> MusicTheoryEngine::buildTriad(int rootNote, const std::vector<int>& scale, int degree)
{
    if (degree < 0 || degree >= static_cast<int>(scale.size())) {
        return {};
    }
    
    std::vector<int> triad;
    int baseOctave = rootNote + DEFAULT_OCTAVE * OCTAVE_SIZE;
    
    // Root
    triad.push_back(baseOctave + (scale[degree] - scale[0]));
    
    // Third
    int thirdDegree = (degree + 2) % scale.size();
    int thirdNote = baseOctave + (scale[thirdDegree] - scale[0]);
    if (thirdNote <= triad[0]) {
        thirdNote += OCTAVE_SIZE;
    }
    triad.push_back(thirdNote);
    
    // Fifth
    int fifthDegree = (degree + 4) % scale.size();
    int fifthNote = baseOctave + (scale[fifthDegree] - scale[0]);
    if (fifthNote <= triad[1]) {
        fifthNote += OCTAVE_SIZE;
    }
    triad.push_back(fifthNote);
    
    return triad;
}

std::vector<int> MusicTheoryEngine::applyExtension(
    const std::vector<int>& triad,
    const juce::String& extension,
    const std::vector<int>& scale,
    int degree)
{
    auto chord = triad;
    
    if (extension == "+6th") {
        // Add 6th (scale degree 6 from root)
        int sixthDegree = (degree + 5) % scale.size();
        int sixthNote = chord[0] + (scale[sixthDegree] - scale[degree % scale.size()]);
        while (sixthNote <= chord.back()) {
            sixthNote += OCTAVE_SIZE;
        }
        chord.push_back(sixthNote);
    }
    else if (extension == "+7th") {
        // Add 7th (scale degree 7 from root)
        int seventhDegree = (degree + 6) % scale.size();
        int seventhNote = chord[0] + (scale[seventhDegree] - scale[degree % scale.size()]);
        while (seventhNote <= chord.back()) {
            seventhNote += OCTAVE_SIZE;
        }
        chord.push_back(seventhNote);
    }
    else if (extension == "+9th") {
        // Add 9th (scale degree 2, octave up)
        int ninthDegree = (degree + 1) % scale.size();
        int ninthNote = chord[0] + OCTAVE_SIZE + (scale[ninthDegree] - scale[degree % scale.size()]);
        chord.push_back(ninthNote);
    }
    else if (extension == "sus2") {
        // Replace 3rd with 2nd
        if (chord.size() >= 2) {
            int secondDegree = (degree + 1) % scale.size();
            chord[1] = chord[0] + (scale[secondDegree] - scale[degree % scale.size()]);
        }
    }
    else if (extension == "sus4") {
        // Replace 3rd with 4th
        if (chord.size() >= 2) {
            int fourthDegree = (degree + 3) % scale.size();
            chord[1] = chord[0] + (scale[fourthDegree] - scale[degree % scale.size()]);
        }
    }
    
    return chord;
}

std::vector<int> MusicTheoryEngine::applyInversion(
    const std::vector<int>& chord,
    const juce::String& inversion)
{
    if (inversion == "None" || inversion == "Root" || chord.size() < 2) {
        return chord;
    }
    
    auto inverted = chord;
    
    if (inversion == "1st") {
        // Move root up an octave
        inverted[0] += OCTAVE_SIZE;
        std::sort(inverted.begin(), inverted.end());
    }
    else if (inversion == "2nd" && chord.size() >= 3) {
        // Move root and third up an octave
        inverted[0] += OCTAVE_SIZE;
        inverted[1] += OCTAVE_SIZE;
        std::sort(inverted.begin(), inverted.end());
    }
    
    return inverted;
}

std::vector<int> MusicTheoryEngine::applyVoicing(
    const std::vector<int>& chord,
    const juce::String& voicing,
    const CustomVoicingData& customData)
{
    if (voicing == "None" || voicing == "Root") {
        return chord;
    }
    
    if (voicing == "Open") {
        // Open voicing - spread notes across wider range
        auto voiced = chord;
        for (size_t i = 1; i < voiced.size(); ++i) {
            voiced[i] += OCTAVE_SIZE;
        }
        return voiced;
    }
    else if (voicing == "Drop 2") {
        // Drop 2 voicing - move second highest note down an octave
        if (chord.size() >= 3) {
            auto voiced = chord;
            std::sort(voiced.begin(), voiced.end());
            voiced[voiced.size() - 2] -= OCTAVE_SIZE;
            std::sort(voiced.begin(), voiced.end());
            return voiced;
        }
    }
    else if (voicing == "Custom") {
        return getCustomVoicing(chord, customData);
    }
    
    return chord;
}

// Custom voicing algorithms
std::vector<int> MusicTheoryEngine::getCustomVoicing(
    const std::vector<int>& baseChord,
    const CustomVoicingData& voicingData)
{
    switch (voicingData.spreadType) {
        case 0: return applyStackedThirds(baseChord, voicingData);
        case 1: return applyLayeredVoicing(baseChord, voicingData);
        case 2: return applyClosedAboveBass(baseChord, voicingData);
        case 3: return applyNoteVariability(baseChord, voicingData);
        case 4: return applyRootFifthSpread(baseChord, voicingData);
        case 5: return applyRootGuideTones(baseChord, voicingData);
        default: return baseChord;
    }
}

std::vector<int> MusicTheoryEngine::applyStackedThirds(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // Standard closed voicing with specified number of notes
    auto voiced = chord;
    int targetOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    // Adjust to target octave
    voiced = adjustToOctave(voiced, data.position);
    
    // Limit to requested number of notes
    if (static_cast<int>(voiced.size()) > data.numNotes) {
        voiced.resize(data.numNotes);
    }
    
    return voiced;
}

std::vector<int> MusicTheoryEngine::applyLayeredVoicing(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // Bass note + open middle + closed top
    if (chord.size() < 3) return chord;
    
    std::vector<int> voiced;
    int baseOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    // Bass note
    voiced.push_back(baseOctave + (chord[0] % OCTAVE_SIZE));
    
    // Middle notes (open)
    for (size_t i = 1; i < chord.size() - 1 && static_cast<int>(voiced.size()) < data.numNotes; ++i) {
        voiced.push_back(baseOctave + OCTAVE_SIZE + (chord[i] % OCTAVE_SIZE));
    }
    
    // Top note (close to middle)
    if (static_cast<int>(voiced.size()) < data.numNotes && !chord.empty()) {
        voiced.push_back(baseOctave + OCTAVE_SIZE + (chord.back() % OCTAVE_SIZE));
    }
    
    return sortNotes(voiced);
}

std::vector<int> MusicTheoryEngine::applyClosedAboveBass(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // Root in bass, remaining notes in closed position above
    if (chord.empty()) return chord;
    
    std::vector<int> voiced;
    int baseOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    // Bass note
    voiced.push_back(baseOctave + (chord[0] % OCTAVE_SIZE));
    
    // Upper notes in closed position
    int upperOctave = baseOctave + OCTAVE_SIZE;
    for (size_t i = 1; i < chord.size() && static_cast<int>(voiced.size()) < data.numNotes; ++i) {
        voiced.push_back(upperOctave + (chord[i] % OCTAVE_SIZE));
    }
    
    return voiced;
}

std::vector<int> MusicTheoryEngine::applyNoteVariability(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // 70% probability per note (matching Python implementation)
    std::vector<int> voiced;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    int baseOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    for (size_t i = 0; i < chord.size() && static_cast<int>(voiced.size()) < data.numNotes; ++i) {
        if (dis(gen) < 0.7) {  // 70% probability
            voiced.push_back(baseOctave + (chord[i] % OCTAVE_SIZE));
        }
    }
    
    // Ensure at least one note
    if (voiced.empty() && !chord.empty()) {
        voiced.push_back(baseOctave + (chord[0] % OCTAVE_SIZE));
    }
    
    return sortNotes(voiced);
}

std::vector<int> MusicTheoryEngine::applyRootFifthSpread(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // Root + fifth with wide spread
    if (chord.size() < 3) return chord;
    
    std::vector<int> voiced;
    int baseOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    // Root
    voiced.push_back(baseOctave + (chord[0] % OCTAVE_SIZE));
    
    // Fifth (if available)
    if (chord.size() >= 3) {
        voiced.push_back(baseOctave + OCTAVE_SIZE + (chord[2] % OCTAVE_SIZE));
    }
    
    // Add remaining notes up to limit
    for (size_t i = 1; i < chord.size() && static_cast<int>(voiced.size()) < data.numNotes; ++i) {
        if (i != 2) {  // Skip fifth (already added)
            voiced.push_back(baseOctave + OCTAVE_SIZE + (chord[i] % OCTAVE_SIZE));
        }
    }
    
    return sortNotes(voiced);
}

std::vector<int> MusicTheoryEngine::applyRootGuideTones(
    const std::vector<int>& chord,
    const CustomVoicingData& data)
{
    // Root + guide tones (3rd, 7th if available)
    std::vector<int> voiced;
    int baseOctave = BASE_MIDI_NOTE + data.position * OCTAVE_SIZE;
    
    // Root
    if (!chord.empty()) {
        voiced.push_back(baseOctave + (chord[0] % OCTAVE_SIZE));
    }
    
    // Third (guide tone)
    if (chord.size() >= 2 && static_cast<int>(voiced.size()) < data.numNotes) {
        voiced.push_back(baseOctave + (chord[1] % OCTAVE_SIZE));
    }
    
    // Seventh (guide tone) - if chord has extensions
    if (chord.size() >= 4 && static_cast<int>(voiced.size()) < data.numNotes) {
        voiced.push_back(baseOctave + (chord[3] % OCTAVE_SIZE));
    }
    
    return sortNotes(voiced);
}

// Validation functions
bool MusicTheoryEngine::isValidKey(const juce::String& key)
{
    return KEY_TO_SEMITONE.find(key) != KEY_TO_SEMITONE.end();
}

bool MusicTheoryEngine::isValidMode(const juce::String& mode)
{
    return std::find(MODE_NAMES.begin(), MODE_NAMES.end(), mode) != MODE_NAMES.end();
}

bool MusicTheoryEngine::isValidRoman(const juce::String& roman)
{
    return ROMAN_TO_DEGREE.find(roman) != ROMAN_TO_DEGREE.end();
}

bool MusicTheoryEngine::isValidExtension(const juce::String& extension)
{
    return std::find(EXTENSION_NAMES.begin(), EXTENSION_NAMES.end(), extension) != EXTENSION_NAMES.end();
}

bool MusicTheoryEngine::isValidInversion(const juce::String& inversion)
{
    return std::find(INVERSION_NAMES.begin(), INVERSION_NAMES.end(), inversion) != INVERSION_NAMES.end();
}

bool MusicTheoryEngine::isValidVoicing(const juce::String& voicing)
{
    return std::find(VOICING_NAMES.begin(), VOICING_NAMES.end(), voicing) != VOICING_NAMES.end();
}

// Utility functions
juce::String MusicTheoryEngine::getChordDisplayName(const ChordData& chord)
{
    juce::String name = chord.roman;
    
    if (chord.hasExtension()) {
        name += " " + chord.extension;
    }
    if (chord.hasInversion()) {
        name += " " + chord.inversion;
    }
    if (chord.hasVoicing()) {
        name += " " + chord.voicing;
    }
    
    return name;
}

juce::Colour MusicTheoryEngine::getChordColor(const juce::String& roman)
{
    // Color coding matching Python implementation
    if (roman == "I" || roman == "IV" || roman == "V") {
        return juce::Colour(0xff1976d2);  // Blue - Primary chords
    }
    else if (roman == "ii" || roman == "iii" || roman == "vi") {
        return juce::Colour(0xff388e3c);  // Green - Secondary chords
    }
    else if (roman == "vii°") {
        return juce::Colour(0xffd32f2f);  // Red - Diminished
    }
    
    return juce::Colours::grey;
}

int MusicTheoryEngine::getDegreeFromRoman(const juce::String& roman)
{
    auto it = ROMAN_TO_DEGREE.find(roman);
    return (it != ROMAN_TO_DEGREE.end()) ? it->second : -1;
}

bool MusicTheoryEngine::isChordMajor(const juce::String& roman)
{
    return roman == "I" || roman == "IV" || roman == "V";
}

bool MusicTheoryEngine::isChordMinor(const juce::String& roman)
{
    return roman == "ii" || roman == "iii" || roman == "vi";
}

bool MusicTheoryEngine::isChordDiminished(const juce::String& roman)
{
    return roman == "vii°";
}

// Private helper functions
int MusicTheoryEngine::getKeyRootNote(const juce::String& key)
{
    auto it = KEY_TO_SEMITONE.find(key);
    return (it != KEY_TO_SEMITONE.end()) ? it->second : -1;
}

std::vector<int> MusicTheoryEngine::getModeIntervals(const juce::String& mode)
{
    for (size_t i = 0; i < MODE_NAMES.size(); ++i) {
        if (MODE_NAMES[i] == mode) {
            return MODES[i];
        }
    }
    return {};
}

std::vector<int> MusicTheoryEngine::adjustToOctave(const std::vector<int>& notes, int octave)
{
    std::vector<int> adjusted;
    int targetOctave = BASE_MIDI_NOTE + octave * OCTAVE_SIZE;
    
    for (int note : notes) {
        int adjustedNote = targetOctave + (note % OCTAVE_SIZE);
        adjusted.push_back(adjustedNote);
    }
    
    return adjusted;
}

std::vector<int> MusicTheoryEngine::ensureRange(const std::vector<int>& notes, int minNote, int maxNote)
{
    std::vector<int> ranged;
    
    for (int note : notes) {
        int adjustedNote = note;
        while (adjustedNote < minNote) {
            adjustedNote += OCTAVE_SIZE;
        }
        while (adjustedNote > maxNote) {
            adjustedNote -= OCTAVE_SIZE;
        }
        ranged.push_back(adjustedNote);
    }
    
    return ranged;
}

std::vector<int> MusicTheoryEngine::removeDuplicates(const std::vector<int>& notes)
{
    std::vector<int> unique;
    
    for (int note : notes) {
        if (std::find(unique.begin(), unique.end(), note) == unique.end()) {
            unique.push_back(note);
        }
    }
    
    return unique;
}

std::vector<int> MusicTheoryEngine::sortNotes(const std::vector<int>& notes)
{
    auto sorted = notes;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

} // namespace ChordFoundry