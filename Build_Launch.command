#!/bin/bash
cd "$(dirname "$0")" || exit 1

echo "Launching Chord Foundry..."

APP_PATH="ChordFoundry/build/bin/Chord Foundry.app"

if [ -d "$APP_PATH" ]; then
    echo "Found Chord Foundry at: $APP_PATH"
    echo "Launching..."
    open "$APP_PATH"
else
    echo "Chord Foundry app not found at: $APP_PATH"
    echo ""
    echo "Build it first:"
    echo "  ./Build_Create.command"
    exit 1
fi
