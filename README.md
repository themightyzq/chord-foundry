# Chord Foundry

Chord Foundry is a desktop app for writing chord progressions and exporting them as
MIDI. You pick chords from a wheel of scale degrees, set extensions and voicings, lay
the chords out on a 32-step pattern grid, optionally arpeggiate them, and export a
`.mid` file to use in any DAW. macOS and Windows. Built with JUCE.

Chords and the click track play back through the default audio output device via a
built-in synth, so you can audition a progression before exporting it.

## Install

Download the latest build for your platform from the
[Releases page](https://github.com/themightyzq/chord-foundry/releases/latest).
The macOS build is universal (Apple Silicon and Intel) and needs macOS 11.0
or later. The builds are unsigned: on macOS, right-click the app and choose
Open the first time. Or build from source (below).

## Use

1. Choose a key and a mode in the settings panel. Fifteen modes are available (Major,
   Dorian, Phrygian, Lydian, and so on), in all twelve keys.
2. Click a slice of the chord wheel to pick a scale degree. Slices are coloured by
   function (primary, secondary, diminished).
3. Press Add Chord to put the chord in the progression. Select a chord in the progression
   and press Modifiers... (or double-click or right-click the chord) to set its extension
   (+6th, +7th, +9th, sus2, sus4), inversion (root, 1st, 2nd), voicing (root, open, drop 2,
   custom) and arpeggiator. Preview plays the chord with the settings before you apply them.
4. Pick a chord in the progression, then drag across empty steps on the pattern grid to draw
   a block of that chord. Drag a block to move it, drag the right edge of its last step to
   resize it, click it or right-click it to remove it. Blocks of the same chord cannot
   overlap; blocks of different chords can, and then sound together. The grid is 32 steps.
5. Optionally pick an arpeggio mode for a chord in the Modifiers dialog: Up, Down, Random,
   Converge, Diverge, Ascending, or Descending, with a note length of 1/16, 1/8, 1/4 or 1/2
   at the set tempo. A Random arpeggio keeps the same order for a given block until that
   block's chord or position changes, and the exported MIDI file uses the same order.
6. Export to MIDI. The file contains the full pattern, with arpeggios written out as
   individual notes.

## Projects

New, Open, Save and Save As are in the File menu and the buttons at the top left of the
window (Cmd or Ctrl plus N, O, S, and Shift+S). A project file (`.cfproj`, XML) holds the
chords with their modifiers and arpeggiator settings, the pattern blocks, the tempo, key,
mode, loop and click-track settings, and the synth volume. Saving writes a temporary file
first and then swaps it in, so a failed save does not damage an existing project. The window
title shows an asterisk when there are unsaved changes, and Chord Foundry asks before you
quit, open another project or start a new one with unsaved changes. A project saved by a
newer version of the app is refused with a message rather than half loaded.

## Playback

Playback counts steps in samples on the audio thread, so steps land at the set tempo without
drifting. One step is a 16th note. A chord that sits on consecutive steps is struck once and
held, and an arpeggio keeps its place across those steps; it is struck again when the chord
changes. To strike the same chord again at the start of a block, Shift-click the first step
of that block (a small marker shows on it).

If no audio output device can be opened, a message says so and offers Audio Settings, where
you can choose the output device, sample rate and buffer size. The same dialog is in the Audio
menu and behind the Audio Settings button.

The window can be as small as 1200 x 760. Below the size the layout needs, it scrolls instead
of clipping.

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
