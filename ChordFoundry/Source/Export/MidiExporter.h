#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Data/ChordProgression.h"

namespace ChordFoundry {

class MidiExporter {
public:
    static bool exportToFile(const juce::File& outputFile,
                            const std::vector<BlockData>& blocks,
                            const std::vector<ChordData>& chords,
                            float tempo,
                            const juce::String& key,
                            const juce::String& mode);
                            
private:
    MidiExporter() = delete; // Static class only
    
    // Helper function to get MIDI notes for a chord
    static std::vector<int> getMidiNotesForChord(const ChordData& chord,
                                                 const juce::String& key,
                                                 const juce::String& mode);
};

} // namespace ChordFoundry