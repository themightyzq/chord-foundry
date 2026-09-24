#include "ArpeggiatorEngine.h"

#include <algorithm>
#include <random>

namespace ChordFoundry {

// Ported from archive/core/arpeggiator.py: get_arpeggio_sequence(notes, mode)
std::vector<float> ArpeggiatorEngine::getArpeggioSequence(const std::vector<float>& notes, const juce::String& mode)
{
    if (notes.empty() || mode.isEmpty() || mode == "None")
        return notes;

    if (mode == "Up")
        return notes;

    if (mode == "Down")
        return std::vector<float>(notes.rbegin(), notes.rend());

    if (mode == "Random")
    {
        std::vector<float> result(notes);
        static thread_local std::mt19937 rng { std::random_device {}() };
        std::shuffle(result.begin(), result.end(), rng);
        return result;
    }

    if (mode == "Converge")
    {
        // [notes[0], notes[-1]] + notes[1:-1]
        if (notes.size() < 2)
            return notes;

        std::vector<float> result;
        result.reserve(notes.size());
        result.push_back(notes.front());
        result.push_back(notes.back());
        result.insert(result.end(), notes.begin() + 1, notes.end() - 1);
        return result;
    }

    if (mode == "Diverge")
    {
        // mid = len // 2; left = notes[:mid][::-1]; right = notes[mid+1:]
        // return [notes[mid]] + left + right
        if (notes.size() < 2)
            return notes;

        const size_t mid = notes.size() / 2;

        std::vector<float> left(notes.begin(), notes.begin() + static_cast<long>(mid));
        std::reverse(left.begin(), left.end());

        std::vector<float> right(notes.begin() + static_cast<long>(mid) + 1, notes.end());

        std::vector<float> result;
        result.reserve(notes.size());
        result.push_back(notes[mid]);
        result.insert(result.end(), left.begin(), left.end());
        result.insert(result.end(), right.begin(), right.end());
        return result;
    }

    if (mode == "Ascending")
    {
        std::vector<float> result(notes);
        std::sort(result.begin(), result.end());
        return result;
    }

    if (mode == "Descending")
    {
        std::vector<float> result(notes);
        std::sort(result.begin(), result.end(), std::greater<float>());
        return result;
    }

    // Unrecognised mode: pass through unchanged, matching the Python fallback.
    return notes;
}

// Ported from archive/core/arpeggiator.py: get_note_duration(arp_length, tempo)
double ArpeggiatorEngine::getNoteLength(const juce::String& arpLength, float tempo)
{
    // duration_map, in units of quarter-note beats. Default (unrecognised
    // arpLength) is "1/16", matching duration_map.get(arp_length, 0.25).
    double multiplier = 0.25;

    if (arpLength == "1/16")
        multiplier = 0.25;
    else if (arpLength == "1/8")
        multiplier = 0.5;
    else if (arpLength == "1/4")
        multiplier = 1.0;
    else if (arpLength == "1/2")
        multiplier = 2.0;

    const double beatsPerSecond = static_cast<double>(tempo) / 60.0;
    const double beatDuration = 1.0 / beatsPerSecond;

    return multiplier * beatDuration;
}

} // namespace ChordFoundry
