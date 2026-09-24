#!/bin/bash
cd "$(dirname "$0")"

echo "🔥 Building Chord Foundry..."

# JUCE is fetched automatically by CMake (FetchContent); nothing to set up here.

# Function to build with CMake
build_with_cmake() {
    echo "📦 Building with CMake..."
    
    if [ ! -d "ChordFoundry" ]; then
        echo "❌ ChordFoundry project not found. Run setup_chord_foundry.sh first."
        exit 1
    fi
    
    cd ChordFoundry
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with CMake
    echo "⚙️  Configuring project..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # Build the project
    echo "🔨 Building project..."
    cmake --build . --config Release -j$(sysctl -n hw.ncpu)
    
    if [ $? -eq 0 ]; then
        echo "✅ Build successful!"
        
        # List built targets
        echo "📋 Built targets:"
        find . -name "*.app" -o -name "*.vst3" -o -name "*.component" | head -10
        
        # Copy to convenient location
        mkdir -p ../Builds/Release
        find . -name "*.app" -exec cp -r {} ../Builds/Release/ \;
        find . -name "*.vst3" -exec cp -r {} ../Builds/Release/ \;
        find . -name "*.component" -exec cp -r {} ../Builds/Release/ \;
        
        echo "📁 Binaries copied to ChordFoundry/Builds/Release/"
    else
        echo "❌ Build failed!"
        exit 1
    fi
}

# Function to build with Xcode (fallback)
build_with_xcode() {
    echo "🍎 Building with Xcode..."
    
    if [ ! -d "ChordFoundry/Builds/MacOSX" ]; then
        echo "❌ Xcode project not found. Generate with Projucer first."
        exit 1
    fi
    
    cd ChordFoundry/Builds/MacOSX
    
    # Try to build with xcodebuild
    xcodebuild -configuration Release -jobs $(sysctl -n hw.ncpu)
    
    if [ $? -eq 0 ]; then
        echo "✅ Xcode build successful!"
        
        # Show built products
        echo "📋 Built products:"
        find build/Release -name "*.app" -o -name "*.vst3" -o -name "*.component" | head -10
    else
        echo "❌ Xcode build failed!"
        exit 1
    fi
}

# Main build logic
if [ -f "ChordFoundry/CMakeLists.txt" ]; then
    build_with_cmake
elif [ -f "ChordFoundry/ChordFoundry.jucer" ]; then
    echo "⚠️  CMakeLists.txt not found, trying Xcode build..."
    build_with_xcode
else
    echo "❌ No ChordFoundry project found!"
    echo "Run setup_chord_foundry.sh first to create the project."
    exit 1
fi

echo ""
echo "🔥 Chord Foundry build complete!"
echo "Run ./run_chord_foundry.command to launch the application."