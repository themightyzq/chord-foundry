#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <memory>

namespace ChordFoundry {

struct CustomVoicingData {
    int numNotes = 3;
    int position = 3;     // Octave position (0-7)
    int spreadType = 0;   // Spread algorithm index (0-5)
    
    bool operator==(const CustomVoicingData& other) const {
        return numNotes == other.numNotes && 
               position == other.position && 
               spreadType == other.spreadType;
    }
};

struct ChordData {
    juce::String roman;
    juce::String extension;
    juce::String inversion;
    juce::String voicing;
    juce::String arpMode;
    juce::String arpLength;
    CustomVoicingData customVoicing;
    
    ChordData() = default;
    
    ChordData(const juce::String& romanNumeral)
        : roman(romanNumeral) {}
    
    bool operator==(const ChordData& other) const {
        return roman == other.roman &&
               extension == other.extension &&
               inversion == other.inversion &&
               voicing == other.voicing &&
               arpMode == other.arpMode &&
               arpLength == other.arpLength &&
               customVoicing == other.customVoicing;
    }
    
    bool hasArpeggiator() const {
        return !arpMode.isEmpty() && arpMode != "None";
    }
    
    bool hasExtension() const {
        return !extension.isEmpty() && extension != "None";
    }
    
    bool hasInversion() const {
        return !inversion.isEmpty() && inversion != "None";
    }
    
    bool hasVoicing() const {
        return !voicing.isEmpty() && voicing != "None";
    }
};

struct BlockData {
    int chordIndex = -1;
    int startStep = 0;
    int lengthSteps = 4;
    juce::Colour displayColor;
    ChordData modifierOverrides;  // Optional per-block chord modifications
    bool hasModifierOverrides = false;

    // Sequenced playback sustains a chord across consecutive steps. Set this to strike
    // the chord again at the start of this block even when the step before it held the
    // same chord.
    bool newStrike = false;
    
    BlockData() = default;
    
    BlockData(int chordIdx, int start, int length, juce::Colour color)
        : chordIndex(chordIdx), startStep(start), lengthSteps(length), displayColor(color) {}
    
    bool operator==(const BlockData& other) const {
        return chordIndex == other.chordIndex &&
               startStep == other.startStep &&
               lengthSteps == other.lengthSteps &&
               displayColor == other.displayColor &&
               modifierOverrides == other.modifierOverrides &&
               hasModifierOverrides == other.hasModifierOverrides &&
               newStrike == other.newStrike;
    }
    
    bool overlaps(const BlockData& other) const {
        if (chordIndex != other.chordIndex) return false;
        int thisEnd = startStep + lengthSteps;
        int otherEnd = other.startStep + other.lengthSteps;
        return !(thisEnd <= other.startStep || startStep >= otherEnd);
    }
    
    bool containsStep(int step) const {
        return step >= startStep && step < (startStep + lengthSteps);
    }
    
    // Get effective chord data (base chord + any overrides)
    ChordData getEffectiveChordData(const ChordData& baseChord) const {
        if (!hasModifierOverrides) {
            return baseChord;
        }
        
        ChordData effective = baseChord;
        
        // Apply overrides only for non-empty values
        if (!modifierOverrides.extension.isEmpty()) {
            effective.extension = modifierOverrides.extension;
        }
        if (!modifierOverrides.inversion.isEmpty()) {
            effective.inversion = modifierOverrides.inversion;
        }
        if (!modifierOverrides.voicing.isEmpty()) {
            effective.voicing = modifierOverrides.voicing;
        }
        if (!modifierOverrides.arpMode.isEmpty()) {
            effective.arpMode = modifierOverrides.arpMode;
        }
        if (!modifierOverrides.arpLength.isEmpty()) {
            effective.arpLength = modifierOverrides.arpLength;
        }
        
        return effective;
    }
};

class ChordProgression {
public:
    ChordProgression() = default;
    ~ChordProgression() = default;
    
    // Chord management
    void addChord(const ChordData& chord);
    void removeChord(int index);
    // Replaces the chord at index (its modifiers, arpeggiator and voicing settings). Pattern
    // blocks refer to chords by index, so they follow the change. Returns false for a bad index.
    bool updateChord(int index, const ChordData& newData);
    void clearChords();
    void randomizeChords();
    
    // Chord access
    const std::vector<ChordData>& getChords() const { return chords; }
    const ChordData& getChord(int index) const;
    int getChordCount() const { return static_cast<int>(chords.size()); }
    bool hasChords() const { return !chords.empty(); }
    
    // Pattern block management
    void addBlock(const BlockData& block);
    void removeBlock(int blockIndex);
    // Replaces the block at blockIndex (for example to change its modifiers or strike flag).
    // Ignored if the new block would be invalid or overlap another block of the same chord.
    void replaceBlock(int blockIndex, const BlockData& newBlock);
    void clearBlocks();
    void randomizeBlocks(int minBlocks, int maxBlocks);
    
    // Block access
    const std::vector<BlockData>& getBlocks() const { return patternBlocks; }
    std::vector<BlockData> getBlocksAtStep(int step) const;
    int getBlockCount() const { return static_cast<int>(patternBlocks.size()); }
    bool hasBlocks() const { return !patternBlocks.empty(); }
    
    // Block operations
    int findBlockAt(int chordIndex, int step) const;
    bool canPlaceBlock(int chordIndex, int startStep, int lengthSteps) const;
    // Placement rules shared by the pattern editor's drag handling and the model. Blocks of
    // the same chord may not overlap; blocks of different chords may (they sound together).
    static bool isPlacementFree(const std::vector<BlockData>& blocks, int ignoreIndex, const BlockData& candidate);
    // Start step closest to desiredStart that keeps the whole block on the 32-step grid.
    static int clampMoveStart(const BlockData& block, int desiredStart);
    // Longest the block at `index` can be made without running into the next block of the
    // same chord or off the end of the grid.
    static int maxResizeLength(const std::vector<BlockData>& blocks, int index);
    // How many steps (up to desiredLength) a new block of `chordIndex` starting at startStep
    // can cover before it would reach another block of that chord. 0 if startStep is taken.
    static int freeRunLength(const std::vector<BlockData>& blocks, int chordIndex, int startStep, int desiredLength);

    void moveBlock(int blockIndex, int newStartStep);
    void resizeBlock(int blockIndex, int newLength);
    
    // Pattern validation
    static constexpr int MAX_CHORDS = 8;
    static constexpr int MAX_STEPS = 32;
    
    bool isValidChordIndex(int index) const {
        return index >= 0 && index < getChordCount();
    }
    
    bool isValidStep(int step) const {
        return step >= 0 && step < MAX_STEPS;
    }
    
    // State management
    void setCurrentStep(int step) { currentStep = juce::jlimit(0, MAX_STEPS - 1, step); }
    int getCurrentStep() const { return currentStep; }
    
    // Serialization helpers
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    std::vector<ChordData> chords;
    std::vector<BlockData> patternBlocks;
    int currentStep = 0;
    
    // Default block colors for each chord index
    static const juce::Array<juce::Colour> DEFAULT_BLOCK_COLORS;
    
    juce::Colour getDefaultColorForChord(int chordIndex) const;
    void validateBlockData();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordProgression)
};

} // namespace ChordFoundry