#!/bin/bash
cd "$(dirname "$0")"

echo "🔥 Launching Chord Foundry..."

# Function to find and run the standalone app
run_standalone() {
    echo "🎵 Looking for Chord Foundry standalone app..."
    
    # Search for the app in common locations
    APP_LOCATIONS=(
        "ChordFoundry/build/bin/Chord Foundry.app"
        "ChordFoundry/Builds/Release/ChordFoundry.app"
        "ChordFoundry/build/ChordFoundry_artefacts/Release/ChordFoundry.app"
        "ChordFoundry/Builds/MacOSX/build/Release/ChordFoundry.app"
        "ChordFoundry/cmake-build-release/ChordFoundry_artefacts/Release/ChordFoundry.app"
    )
    
    for app_path in "${APP_LOCATIONS[@]}"; do
        if [ -d "$app_path" ]; then
            echo "✅ Found Chord Foundry at: $app_path"
            echo "🚀 Launching..."
            open "$app_path"
            return 0
        fi
    done
    
    return 1
}

# Function to run development version
run_dev() {
    echo "🛠️  Looking for development build..."
    
    DEV_LOCATIONS=(
        "ChordFoundry/build/bin/Chord Foundry.app"
        "ChordFoundry/build/ChordFoundry_artefacts/Debug/ChordFoundry.app"
        "ChordFoundry/Builds/MacOSX/build/Debug/ChordFoundry.app"
    )
    
    for app_path in "${DEV_LOCATIONS[@]}"; do
        if [ -d "$app_path" ]; then
            echo "✅ Found development build at: $app_path"
            echo "🚀 Launching development version..."
            open "$app_path"
            return 0
        fi
    done
    
    return 1
}

# Function to show plugin information
show_plugins() {
    echo "🎛️  Available plugin formats:"
    
    PLUGIN_LOCATIONS=(
        "ChordFoundry/Builds/Release/*.vst3"
        "ChordFoundry/Builds/Release/*.component"
        "ChordFoundry/build/ChordFoundry_artefacts/Release/*.vst3"
        "ChordFoundry/build/ChordFoundry_artefacts/Release/*.component"
    )
    
    local found_plugins=false
    
    for plugin_pattern in "${PLUGIN_LOCATIONS[@]}"; do
        for plugin_path in $plugin_pattern; do
            if [ -e "$plugin_path" ]; then
                echo "  📦 $(basename "$plugin_path")"
                found_plugins=true
            fi
        done
    done
    
    if [ "$found_plugins" = true ]; then
        echo ""
        echo "💡 To use plugins:"
        echo "   - VST3: Copy .vst3 files to ~/Library/Audio/Plug-Ins/VST3/"
        echo "   - AU: Copy .component files to ~/Library/Audio/Plug-Ins/Components/"
        echo ""
    fi
}

# Main execution
if [ "$1" = "--dev" ]; then
    # Try to run development version first
    if ! run_dev; then
        echo "⚠️  Development build not found, trying release version..."
        run_standalone
    fi
elif [ "$1" = "--plugins" ]; then
    show_plugins
else
    # Try to run release version first
    if ! run_standalone; then
        echo "⚠️  Release build not found, trying development version..."
        if ! run_dev; then
            echo "❌ No Chord Foundry application found!"
            echo ""
            echo "Please build the project first:"
            echo "  ./build_chord_foundry.command"
            echo ""
            echo "Or check for plugins:"
            echo "  ./run_chord_foundry.command --plugins"
            exit 1
        fi
    fi
fi

echo ""
echo "🎵 Enjoy forging chords with Chord Foundry!"