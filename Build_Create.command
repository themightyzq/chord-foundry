#!/bin/bash
cd "$(dirname "$0")" || exit 1

echo "Building Chord Foundry..."

if [ ! -f "ChordFoundry/CMakeLists.txt" ]; then
    echo "ChordFoundry/CMakeLists.txt not found."
    exit 1
fi

echo "Configuring project..."
cmake -S ChordFoundry -B ChordFoundry/build -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
if cmake --build ChordFoundry/build --config Release -j"$(sysctl -n hw.ncpu)"; then
    echo "Build successful."
    echo "App location: ChordFoundry/build/bin/Chord Foundry.app"
else
    echo "Build failed."
    exit 1
fi

echo ""
echo "Chord Foundry build complete."
echo "Run ./Build_Launch.command to launch the application."
