#include "ChordProgression.h"
#include <random>
#include <algorithm>

namespace ChordFoundry {

const juce::Array<juce::Colour> ChordProgression::DEFAULT_BLOCK_COLORS = {
    juce::Colour(0xff1976d2),  // Blue - Primary chords (I, IV, V)
    juce::Colour(0xff388e3c),  // Green - Secondary chords (ii, iii, vi)
    juce::Colour(0xff7b1fa2),  // Purple
    juce::Colour(0xfff57c00),  // Orange
    juce::Colour(0xffd32f2f),  // Red - Diminished (vii°)
    juce::Colour(0xff5d4037),  // Brown
    juce::Colour(0xff455a64),  // Blue Grey
    juce::Colour(0xff8e24aa)   // Deep Purple
};

// Chord management
void ChordProgression::addChord(const ChordData& chord) {
    if (chords.size() < MAX_CHORDS) {
        chords.push_back(chord);
    }
}

void ChordProgression::removeChord(int index) {
    if (isValidChordIndex(index)) {
        chords.erase(chords.begin() + index);
        
        // Remove any blocks referencing this chord
        patternBlocks.erase(
            std::remove_if(patternBlocks.begin(), patternBlocks.end(),
                [index](const BlockData& block) {
                    return block.chordIndex == index;
                }),
            patternBlocks.end()
        );
        
        // Update remaining block indices
        for (auto& block : patternBlocks) {
            if (block.chordIndex > index) {
                block.chordIndex--;
            }
        }
    }
}

void ChordProgression::clearChords() {
    chords.clear();
    patternBlocks.clear();
}

void ChordProgression::randomizeChords() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(chords.begin(), chords.end(), g);
}

const ChordData& ChordProgression::getChord(int index) const {
    static ChordData emptyChord;
    if (isValidChordIndex(index)) {
        return chords[index];
    }
    return emptyChord;
}

// Pattern block management
void ChordProgression::addBlock(const BlockData& block) {
    if (isValidChordIndex(block.chordIndex) && 
        isValidStep(block.startStep) &&
        block.lengthSteps > 0 &&
        (block.startStep + block.lengthSteps) <= MAX_STEPS) {
        
        patternBlocks.push_back(block);
        validateBlockData();
    }
}

void ChordProgression::removeBlock(int blockIndex) {
    if (blockIndex >= 0 && blockIndex < static_cast<int>(patternBlocks.size())) {
        patternBlocks.erase(patternBlocks.begin() + blockIndex);
    }
}

void ChordProgression::clearBlocks() {
    patternBlocks.clear();
}

void ChordProgression::randomizeBlocks(int minBlocks, int maxBlocks) {
    clearBlocks();
    
    if (chords.empty()) return;
    
    juce::Random random;
    int targetBlocks = random.nextInt(juce::Range<int>(minBlocks, maxBlocks + 1));
    
    std::vector<int> availableDurations = {1, 2, 4, 6, 8};
    
    for (int i = 0; i < targetBlocks; ++i) {
        int attempts = 0;
        const int maxAttempts = 50;
        
        while (attempts < maxAttempts) {
            int chordIndex = random.nextInt(static_cast<int>(chords.size()));
            int startStep = random.nextInt(MAX_STEPS);
            int duration = availableDurations[random.nextInt(static_cast<int>(availableDurations.size()))];
            
            if (startStep + duration <= MAX_STEPS && 
                canPlaceBlock(chordIndex, startStep, duration)) {
                
                BlockData newBlock(chordIndex, startStep, duration, 
                                 getDefaultColorForChord(chordIndex));
                patternBlocks.push_back(newBlock);
                break;
            }
            attempts++;
        }
    }
}

std::vector<BlockData> ChordProgression::getBlocksAtStep(int step) const {
    std::vector<BlockData> blocksAtStep;
    
    for (const auto& block : patternBlocks) {
        if (block.containsStep(step)) {
            blocksAtStep.push_back(block);
        }
    }
    
    return blocksAtStep;
}

int ChordProgression::findBlockAt(int chordIndex, int step) const {
    for (int i = 0; i < static_cast<int>(patternBlocks.size()); ++i) {
        const auto& block = patternBlocks[i];
        if (block.chordIndex == chordIndex && block.containsStep(step)) {
            return i;
        }
    }
    return -1;
}

bool ChordProgression::canPlaceBlock(int chordIndex, int startStep, int lengthSteps) const {
    if (!isValidChordIndex(chordIndex) || !isValidStep(startStep) || 
        lengthSteps <= 0 || (startStep + lengthSteps) > MAX_STEPS) {
        return false;
    }
    
    BlockData testBlock(chordIndex, startStep, lengthSteps, juce::Colours::white);
    
    for (const auto& existingBlock : patternBlocks) {
        if (testBlock.overlaps(existingBlock)) {
            return false;
        }
    }
    
    return true;
}

void ChordProgression::moveBlock(int blockIndex, int newStartStep) {
    if (blockIndex >= 0 && blockIndex < static_cast<int>(patternBlocks.size())) {
        auto& block = patternBlocks[blockIndex];
        
        if (isValidStep(newStartStep) && 
            (newStartStep + block.lengthSteps) <= MAX_STEPS) {
            
            // Temporarily remove the block to test placement
            BlockData tempBlock = block;
            tempBlock.startStep = newStartStep;
            
            // Check if new position is valid (ignoring the current block)
            bool canMove = true;
            for (int i = 0; i < static_cast<int>(patternBlocks.size()); ++i) {
                if (i != blockIndex && tempBlock.overlaps(patternBlocks[i])) {
                    canMove = false;
                    break;
                }
            }
            
            if (canMove) {
                block.startStep = newStartStep;
            }
        }
    }
}

void ChordProgression::resizeBlock(int blockIndex, int newLength) {
    if (blockIndex >= 0 && blockIndex < static_cast<int>(patternBlocks.size())) {
        auto& block = patternBlocks[blockIndex];
        
        if (newLength > 0 && (block.startStep + newLength) <= MAX_STEPS) {
            // Test if resized block would overlap with others
            BlockData tempBlock = block;
            tempBlock.lengthSteps = newLength;
            
            bool canResize = true;
            for (int i = 0; i < static_cast<int>(patternBlocks.size()); ++i) {
                if (i != blockIndex && tempBlock.overlaps(patternBlocks[i])) {
                    canResize = false;
                    break;
                }
            }
            
            if (canResize) {
                block.lengthSteps = newLength;
            }
        }
    }
}

juce::Colour ChordProgression::getDefaultColorForChord(int chordIndex) const {
    if (chordIndex >= 0 && chordIndex < DEFAULT_BLOCK_COLORS.size()) {
        return DEFAULT_BLOCK_COLORS[chordIndex];
    }
    return juce::Colours::grey;
}

void ChordProgression::validateBlockData() {
    // Remove invalid blocks
    patternBlocks.erase(
        std::remove_if(patternBlocks.begin(), patternBlocks.end(),
            [this](const BlockData& block) {
                return !isValidChordIndex(block.chordIndex) ||
                       !isValidStep(block.startStep) ||
                       block.lengthSteps <= 0 ||
                       (block.startStep + block.lengthSteps) > MAX_STEPS;
            }),
        patternBlocks.end()
    );
}

// Serialization
juce::ValueTree ChordProgression::toValueTree() const {
    juce::ValueTree tree("ChordProgression");
    
    // Save chords
    juce::ValueTree chordsTree("Chords");
    for (const auto& chord : chords) {
        juce::ValueTree chordTree("Chord");
        chordTree.setProperty("roman", chord.roman, nullptr);
        chordTree.setProperty("extension", chord.extension, nullptr);
        chordTree.setProperty("inversion", chord.inversion, nullptr);
        chordTree.setProperty("voicing", chord.voicing, nullptr);
        chordTree.setProperty("arpMode", chord.arpMode, nullptr);
        chordTree.setProperty("arpLength", chord.arpLength, nullptr);
        
        // Custom voicing data
        juce::ValueTree voicingTree("CustomVoicing");
        voicingTree.setProperty("numNotes", chord.customVoicing.numNotes, nullptr);
        voicingTree.setProperty("position", chord.customVoicing.position, nullptr);
        voicingTree.setProperty("spreadType", chord.customVoicing.spreadType, nullptr);
        chordTree.appendChild(voicingTree, nullptr);
        
        chordsTree.appendChild(chordTree, nullptr);
    }
    tree.appendChild(chordsTree, nullptr);
    
    // Save pattern blocks
    juce::ValueTree blocksTree("PatternBlocks");
    for (const auto& block : patternBlocks) {
        juce::ValueTree blockTree("Block");
        blockTree.setProperty("chordIndex", block.chordIndex, nullptr);
        blockTree.setProperty("startStep", block.startStep, nullptr);
        blockTree.setProperty("lengthSteps", block.lengthSteps, nullptr);
        blockTree.setProperty("displayColor", static_cast<int>(block.displayColor.getARGB()), nullptr);
        blockTree.setProperty("hasModifierOverrides", block.hasModifierOverrides, nullptr);
        
        // Save modifier overrides if present
        if (block.hasModifierOverrides) {
            juce::ValueTree overridesTree("ModifierOverrides");
            overridesTree.setProperty("roman", block.modifierOverrides.roman, nullptr);
            overridesTree.setProperty("extension", block.modifierOverrides.extension, nullptr);
            overridesTree.setProperty("inversion", block.modifierOverrides.inversion, nullptr);
            overridesTree.setProperty("voicing", block.modifierOverrides.voicing, nullptr);
            overridesTree.setProperty("arpMode", block.modifierOverrides.arpMode, nullptr);
            overridesTree.setProperty("arpLength", block.modifierOverrides.arpLength, nullptr);
            blockTree.appendChild(overridesTree, nullptr);
        }
        
        blocksTree.appendChild(blockTree, nullptr);
    }
    tree.appendChild(blocksTree, nullptr);
    
    return tree;
}

void ChordProgression::fromValueTree(const juce::ValueTree& tree) {
    clearChords();
    
    // Load chords
    auto chordsTree = tree.getChildWithName("Chords");
    for (int i = 0; i < chordsTree.getNumChildren(); ++i) {
        auto chordTree = chordsTree.getChild(i);
        
        ChordData chord;
        chord.roman = chordTree.getProperty("roman", "");
        chord.extension = chordTree.getProperty("extension", "");
        chord.inversion = chordTree.getProperty("inversion", "");
        chord.voicing = chordTree.getProperty("voicing", "");
        chord.arpMode = chordTree.getProperty("arpMode", "");
        chord.arpLength = chordTree.getProperty("arpLength", "");
        
        // Load custom voicing data
        auto voicingTree = chordTree.getChildWithName("CustomVoicing");
        if (voicingTree.isValid()) {
            chord.customVoicing.numNotes = voicingTree.getProperty("numNotes", 3);
            chord.customVoicing.position = voicingTree.getProperty("position", 3);
            chord.customVoicing.spreadType = voicingTree.getProperty("spreadType", 0);
        }
        
        chords.push_back(chord);
    }
    
    // Load pattern blocks
    auto blocksTree = tree.getChildWithName("PatternBlocks");
    for (int i = 0; i < blocksTree.getNumChildren(); ++i) {
        auto blockTree = blocksTree.getChild(i);
        
        BlockData block;
        block.chordIndex = blockTree.getProperty("chordIndex", -1);
        block.startStep = blockTree.getProperty("startStep", 0);
        block.lengthSteps = blockTree.getProperty("lengthSteps", 4);
        block.displayColor = juce::Colour(static_cast<juce::uint32>(static_cast<int>(blockTree.getProperty("displayColor", static_cast<int>(0xff1976d2)))));
        block.hasModifierOverrides = blockTree.getProperty("hasModifierOverrides", false);
        
        // Load modifier overrides if present
        if (block.hasModifierOverrides) {
            auto overridesTree = blockTree.getChildWithName("ModifierOverrides");
            if (overridesTree.isValid()) {
                block.modifierOverrides.roman = overridesTree.getProperty("roman", "");
                block.modifierOverrides.extension = overridesTree.getProperty("extension", "");
                block.modifierOverrides.inversion = overridesTree.getProperty("inversion", "");
                block.modifierOverrides.voicing = overridesTree.getProperty("voicing", "");
                block.modifierOverrides.arpMode = overridesTree.getProperty("arpMode", "");
                block.modifierOverrides.arpLength = overridesTree.getProperty("arpLength", "");
            }
        }
        
        patternBlocks.push_back(block);
    }
    
    validateBlockData();
}

} // namespace ChordFoundry