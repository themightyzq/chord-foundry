# Chord Foundry

Chord Foundry is a desktop app for writing chord progressions and exporting them as
MIDI. You pick chords from a wheel of scale degrees, set extensions and voicings, lay
the chords out on a 32-step pattern grid, optionally arpeggiate them, and export a
`.mid` file to use in any DAW. macOS and Windows. Built with JUCE.

Chords and the click track play back through the default audio output device via a
built-in synth, so you can audition a progression before exporting it.

## Install

There are no packaged releases yet. Build from source (below).

## Use

1. Choose a key and a mode in the settings panel. Fifteen modes are available (Major,
   Dorian, Phrygian, Lydian, and so on), in all twelve keys.
2. Click a slice of the chord wheel to pick a scale degree. Slices are coloured by
   function (primary, secondary, diminished).
3. Set the chord's extension (6th, 7th, 9th, sus2, sus4) and voicing (close, open,
   drop 2, spread) with the modifier controls.
4. Drag on the pattern grid to place the chord as a block. Blocks can be moved, resized,
   and given their own modifiers that override the global ones. The grid is 32 steps.
5. Optionally pick an arpeggio mode per block: Up, Down, Random, Converge, Diverge,
   Ascending, or Descending, with a note length of 1/16, 1/8, 1/4 or 1/2 at the set tempo.
6. Export to MIDI. The file contains the full pattern, with arpeggios written out as
   individual notes.

## Build from source

Requirements: CMake 3.22 or newer and a C++17 compiler (Xcode command-line tools on macOS,
Visual Studio 2022 on Windows). CMake fetches JUCE itself; there is nothing to install
beforehand.

Clone the repository, then:

```
cmake -S ChordFoundry -B ChordFoundry/build -DCMAKE_BUILD_TYPE=Release
cmake --build ChordFoundry/build -j
```

The app is written to `ChordFoundry/build/bin/`. On macOS, `Build_Create.command` and
`Build_Launch.command` are convenience wrappers around the same commands. Tests:
`ctest --test-dir ChordFoundry/build`.

## Licence

GPL-3.0-or-later. See `LICENSE`. Built with JUCE.

ZQ SFX, https://www.zq-sfx.com, connect@zq-sfx.com.
