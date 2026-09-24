cmake_minimum_required(VERSION 3.22)

if(APPLE)
    # Universal binary is mandatory: Soundminer will not load anything else (JUCE/CLAUDE.md section 5).
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
endif()
project(ChordFoundry VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE framework (pinned to commit b272f968aa, tag 8.0.8-146-gb272f968aa; a bare
# commit cannot be shallow-fetched, hence GIT_SHALLOW FALSE).
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        b272f968aa
    GIT_SHALLOW    FALSE
)
FetchContent_MakeAvailable(JUCE)

# Create the main application target
juce_add_gui_app(ChordFoundry
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    BUNDLE_ID "com.zqsfx.chordfoundry"
    PRODUCT_NAME "Chord Foundry"
    VERSION "1.0.0")

# Add source files
target_sources(ChordFoundry PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/MusicTheory/MusicTheoryEngine.cpp
    Source/MusicTheory/MusicTheoryEngine.h
    Source/Audio/ChordSynthesizer.cpp
    Source/Audio/ChordSynthesizer.h
    Source/Audio/ArpeggiatorEngine.cpp
    Source/Audio/ArpeggiatorEngine.h
    Source/UI/ChordPanelComponent.cpp
    Source/UI/ChordPanelComponent.h
    Source/UI/SettingsPanelComponent.cpp
    Source/UI/SettingsPanelComponent.h
    Source/UI/StructurePanelComponent.cpp
    Source/UI/StructurePanelComponent.h
    Source/UI/PatternEditorComponent.cpp
    Source/UI/PatternEditorComponent.h
    Source/UI/ChordModifierDialog.cpp
    Source/UI/ChordModifierDialog.h
    Source/UI/ModernLookAndFeel.cpp
    Source/UI/ModernLookAndFeel.h
    Source/Data/ChordProgression.cpp
    Source/Data/ChordProgression.h
    Source/Export/MidiExporter.cpp
    Source/Export/MidiExporter.h)

# Link required JUCE modules
target_link_libraries(ChordFoundry PRIVATE
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_midi_ci)

# Compiler definitions for JUCE
target_compile_definitions(ChordFoundry PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ChordFoundry,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ChordFoundry,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

# Include directories
target_include_directories(ChordFoundry PRIVATE
    Source
    Source/MusicTheory
    Source/Audio
    Source/UI
    Source/Data
    Source/Export)

# Platform-specific configurations
if(APPLE)
    target_compile_definitions(ChordFoundry PRIVATE
        JUCE_USE_COCOA=1
        JUCE_USE_COREAUDIO=1
        JUCE_USE_COREMIDI=1)
elseif(WIN32)
    target_compile_definitions(ChordFoundry PRIVATE
        JUCE_USE_DIRECTSOUND=1
        JUCE_USE_WASAPI=1
        JUCE_USE_WINMM_MIDI=1)
else()
    target_compile_definitions(ChordFoundry PRIVATE
        JUCE_USE_ALSA=1
        JUCE_USE_ALSA_MIDI=1)
endif()

# Copy any resources
# (Add resource files here if needed)

# Set output directories
set_target_properties(ChordFoundry PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Development flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ChordFoundry PRIVATE
        DEBUG=1
        _DEBUG=1
        JUCE_DEBUG=1)
else()
    target_compile_definitions(ChordFoundry PRIVATE
        NDEBUG=1
        JUCE_DEBUG=0)
endif()

# Enable all warnings for development
if(MSVC)
    target_compile_options(ChordFoundry PRIVATE /W4)
else()
    target_compile_options(ChordFoundry PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Unit tests, run via ctest. Headless: audio DSP (ChordSynthesizer) is
# rendered directly into buffers here, never through a real audio device or
# any GUI JUCE module.
# ---------------------------------------------------------------------------
enable_testing()

juce_add_console_app(ChordFoundryTests
    COMPANY_NAME "ZQ SFX"
    BUNDLE_ID "com.zqsfx.chordfoundrytests"
    PRODUCT_NAME "ChordFoundryTests"
    VERSION "1.0.0")

target_sources(ChordFoundryTests PRIVATE
    Source/Audio/ArpeggiatorEngine.cpp
    Source/Audio/ArpeggiatorEngine.h
    Source/Audio/ChordSynthesizer.cpp
    Source/Audio/ChordSynthesizer.h
    Tests/ArpeggiatorEngineTests.cpp
    Tests/ChordSynthesizerTests.cpp
    Tests/TestMain.cpp)

target_compile_definitions(ChordFoundryTests PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ChordFoundryTests,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ChordFoundryTests,JUCE_VERSION>")

target_include_directories(ChordFoundryTests PRIVATE
    Source
    Source/Audio)

# The console test app has no use for JUCE's curl networking or web browser; leaving them
# on makes juce_core reference libcurl, which the Linux CI runner does not link.
target_compile_definitions(ChordFoundryTests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(ChordFoundryTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_dsp)

add_test(NAME ArpeggiatorEngineTests COMMAND ChordFoundryTests)