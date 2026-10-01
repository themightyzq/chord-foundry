#include "MidiExporter.h"
#include "../MusicTheory/MusicTheoryEngine.h"
#include "../Audio/ArpeggiatorEngine.h"

namespace ChordFoundry {

bool MidiExporter::exportToFile(const juce::File& outputFile,
                               const std::vector<BlockData>& blocks,
                               const std::vector<ChordData>& chords,
                               float tempo,
                               const juce::String& key,
                               const juce::String& mode)
{
    // Create a MIDI file
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(480); // Standard resolution
    
    // Create a single track
    juce::MidiMessageSequence track;
    
    // Add tempo meta event
    // A zero, negative or NaN tempo would divide by zero / produce a nonsense tempo event.
    if (!(tempo > 0.0f))
        tempo = 120.0f;

    // The tempo meta event stores microseconds per quarter note in 24 bits.
    double microsecondsPerQuarterNote = juce::jlimit(1.0, 16777215.0, 60000000.0 / tempo);
    auto tempoEvent = juce::MidiMessage::tempoMetaEvent(static_cast<int>(microsecondsPerQuarterNote));
    track.addEvent(tempoEvent, 0.0);
    
    // Define ticks per step (16th note = 120 ticks at 480 TPQN)
    const int ticksPerSixteenth = 120;
    
    // Structure to hold note events: {time, isNoteOn, noteNumber, velocity}
    struct NoteEvent {
        double time;
        bool isNoteOn;
        int noteNumber;
        int velocity;
    };
    std::vector<NoteEvent> events;
    
    // Process each block
    for (const auto& block : blocks)
    {
        if (block.chordIndex < 0 || block.chordIndex >= static_cast<int>(chords.size()))
            continue;
            
        // Get the chord data
        ChordData chord = chords[block.chordIndex];
        
        // Apply block-specific modifiers if available
        if (block.hasModifierOverrides)
        {
            if (!block.modifierOverrides.extension.isEmpty()) chord.extension = block.modifierOverrides.extension;
            if (!block.modifierOverrides.inversion.isEmpty()) chord.inversion = block.modifierOverrides.inversion;
            if (!block.modifierOverrides.voicing.isEmpty()) chord.voicing = block.modifierOverrides.voicing;
            if (!block.modifierOverrides.arpMode.isEmpty()) chord.arpMode = block.modifierOverrides.arpMode;
            if (!block.modifierOverrides.arpLength.isEmpty()) chord.arpLength = block.modifierOverrides.arpLength;
        }
        
        // Get MIDI notes for this chord
        auto midiNotes = getMidiNotesForChord(chord, key, mode);
        
        // Calculate timing
        double startTime = block.startStep * ticksPerSixteenth;
        double endTime = (block.startStep + block.lengthSteps) * ticksPerSixteenth;
        
        // Handle arpeggiation if enabled
        if (!chord.arpMode.isEmpty() && chord.arpMode != "None")
        {
            // Reorder the chord's notes using the real arpeggiator engine.
            std::vector<float> notesAsFloat(midiNotes.begin(), midiNotes.end());
            ArpeggiatorEngine arpeggiator;
            std::vector<float> arpeggiatedFloat = arpeggiator.getArpeggioSequence(notesAsFloat, chord.arpMode);

            std::vector<int> sequence;
            sequence.reserve(arpeggiatedFloat.size());
            for (float note : arpeggiatedFloat)
                sequence.push_back(static_cast<int>(std::round(note)));

            // An invalid roman numeral/key/mode yields no notes; nothing to arpeggiate.
            if (sequence.empty())
                continue;

            // Map arp length to steps
            int stepsPerNote = 1; // Default to 16th
            if (chord.arpLength == "1/8") stepsPerNote = 2;
            else if (chord.arpLength == "1/4") stepsPerNote = 4;
            else if (chord.arpLength == "1/2") stepsPerNote = 8;
            
            // Generate arpeggiated notes
            int totalSteps = block.lengthSteps;
            int seqIndex = 0;
            
            for (int step = 0; step < totalSteps; step += stepsPerNote)
            {
                int noteNum = sequence[seqIndex % sequence.size()];
                double noteOnTime = startTime + (step * ticksPerSixteenth);
                double noteOffTime = juce::jmin(noteOnTime + (stepsPerNote * ticksPerSixteenth), endTime);
                
                events.push_back({noteOnTime, true, noteNum, 80});
                events.push_back({noteOffTime, false, noteNum, 64});
                
                seqIndex++;
            }
        }
        else
        {
            // Regular block chord - all notes play for the duration
            for (int noteNum : midiNotes)
            {
                events.push_back({startTime, true, noteNum, 80});
                events.push_back({endTime, false, noteNum, 64});
            }
        }
    }
    
    // Sort events by time, with note-offs before note-ons at the same time
    std::sort(events.begin(), events.end(), [](const NoteEvent& a, const NoteEvent& b) {
        if (a.time != b.time)
            return a.time < b.time;
        return !a.isNoteOn && b.isNoteOn; // Note-offs first
    });
    
    // Convert events to MIDI messages
    for (const auto& event : events)
    {
        juce::MidiMessage msg;
        if (event.isNoteOn)
            msg = juce::MidiMessage::noteOn(1, event.noteNumber, static_cast<juce::uint8>(event.velocity));
        else
            msg = juce::MidiMessage::noteOff(1, event.noteNumber, static_cast<juce::uint8>(event.velocity));
            
        track.addEvent(msg, event.time);
    }
    
    // Add the track to the file
    midiFile.addTrack(track);
    
    // Write to a temporary file, then swap it over the target. FileOutputStream on an
    // existing file appends (it positions at the end), so writing straight to
    // outputFile would leave the old bytes in front of the new ones.
    juce::TemporaryFile temp(outputFile);

    {
        auto stream = temp.getFile().createOutputStream();
        if (stream == nullptr || stream->getStatus().failed())
            return false;

        if (!midiFile.writeTo(*stream))
            return false;

        stream->flush();
        if (stream->getStatus().failed())
            return false;

        stream.reset(); // close before the swap
    }

    return temp.overwriteTargetFileWithTemporary();
}

std::vector<int> MidiExporter::getMidiNotesForChord(const ChordData& chord,
                                                    const juce::String& key,
                                                    const juce::String& mode)
{
    // Get the frequencies from the music theory engine
    auto frequencies = MusicTheoryEngine::getChordFrequencies(
        chord.roman, key, mode, chord
    );
    
    // Convert frequencies to MIDI note numbers
    std::vector<int> midiNotes;
    for (float freq : frequencies)
    {
        // MIDI note formula: 69 + 12 * log2(freq/440)
        int midiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(freq / 440.0f)));
        midiNote = juce::jlimit(0, 127, midiNote);
        midiNotes.push_back(midiNote);
    }
    
    return midiNotes;
}

} // namespace ChordFoundry
