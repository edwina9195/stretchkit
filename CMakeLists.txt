cmake_minimum_required(VERSION 3.22)

# macOS: build for 10.13+ (set before project())
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# Windows: link the C++ runtime statically so users do not need the VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(StretchKit VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(STRETCHKIT_BUILD_TESTS "Build the offline DSP test app" OFF)
option(STRETCHKIT_BUILD_HOST_TEST "Build the end-to-end processor/editor test app" OFF)

include(FetchContent)

# JUCE 8 (AGPLv3 or JUCE licence)
FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.10
    GIT_SHALLOW    TRUE)

# Rubber Band Library (GPLv2+). Built from its single-file compilation unit.
FetchContent_Declare(rubberband
    GIT_REPOSITORY https://github.com/breakfastquay/rubberband.git
    GIT_TAG        v4.0.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  _no_cmake_here)

FetchContent_MakeAvailable(juce rubberband)

add_library(rubberband STATIC ${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp)
target_include_directories(rubberband PUBLIC ${rubberband_SOURCE_DIR})
set_target_properties(rubberband PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(APPLE)
    target_link_libraries(rubberband PUBLIC "-framework Accelerate")
endif()
if(MSVC)
    target_compile_definitions(rubberband PRIVATE _USE_MATH_DEFINES NOMINMAX)
    target_compile_options(rubberband PRIVATE /bigobj)
endif()

# ---------------------------------------------------------------------------
# Plugin
# Change COMPANY_NAME / BUNDLE_ID / codes before you publish.
# PLUGIN_MANUFACTURER_CODE: 4 chars, at least one upper-case letter.
# ---------------------------------------------------------------------------
option(STRETCHKIT_BUILD_PLUGIN "Build the plugin" ON)
# Embedded fonts (SIL Open Font License, see Resources/fonts/licenses) and chrome icons (tools/chrome_icons.py)
juce_add_binary_data(StretchKitFonts
    NAMESPACE BinaryData
    HEADER_NAME BinaryData.h
    SOURCES
        Resources/fonts/Rajdhani-Medium.ttf
        Resources/fonts/Rajdhani-SemiBold.ttf
        Resources/fonts/Rajdhani-Bold.ttf
        Resources/fonts/Michroma-Regular.ttf
        Resources/fonts/VT323-Regular.ttf
        Resources/fonts/Silkscreen-Regular.ttf
        Resources/fonts/PressStart2P-Regular.ttf
        Resources/fonts/Inter-Variable.ttf
        Resources/icons/icon_kick.png
        Resources/icons/icon_snare.png
        Resources/icons/icon_clap.png
        Resources/icons/icon_hat.png
        Resources/icons/icon_cymbal.png
        Resources/icons/icon_tom.png
        Resources/icons/icon_perc.png
        Resources/icons/icon_other.png
        Resources/icons/icon_play.png
        Resources/icons/icon_stop.png
        Resources/icons/icon_add.png
        Resources/icons/icon_folder.png
        Resources/icons/icon_rescan.png
        Resources/icons/icon_dice.png
        Resources/icons/icon_midi.png)
set_target_properties(StretchKitFonts PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(STRETCHKIT_UI_SOURCES
    Source/library/SampleLibrary.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/Theme.cpp
    Source/ui/MapView.cpp
    Source/ui/TrackRow.cpp)

set(STRETCHKIT_CORE_SOURCES
    Source/dsp/DrumStretcher.cpp
    Source/library/FeatureExtractor.cpp)

if(STRETCHKIT_BUILD_PLUGIN)
juce_add_plugin(StretchKit
    COMPANY_NAME                "sxdrv.mp3"
    BUNDLE_ID                   "com.stretchkit.stretchkit"
    PRODUCT_NAME                "StretchKit"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Stkt
    PLUGIN_CODE                 Stk1
    FORMATS                     VST3 AU Standalone
    VST3_CATEGORIES             Instrument Drum Sampler
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

target_sources(StretchKit PRIVATE
    ${STRETCHKIT_CORE_SOURCES}
    ${STRETCHKIT_UI_SOURCES})

target_include_directories(StretchKit PRIVATE Source)

target_compile_definitions(StretchKit PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1)

target_link_libraries(StretchKit
    PRIVATE
        rubberband
        StretchKitFonts
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()

# ---------------------------------------------------------------------------
# Offline tests for the stretch + classification code (no GUI, no host)
# ---------------------------------------------------------------------------
if(STRETCHKIT_BUILD_TESTS)
    juce_add_console_app(StretchKitTests PRODUCT_NAME "StretchKitTests")
    target_sources(StretchKitTests PRIVATE ${STRETCHKIT_CORE_SOURCES} Tests/Tests.cpp)
    target_include_directories(StretchKitTests PRIVATE Source)
    target_compile_definitions(StretchKitTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(StretchKitTests PRIVATE
        rubberband juce::juce_audio_formats juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()

if(STRETCHKIT_BUILD_HOST_TEST AND STRETCHKIT_BUILD_PLUGIN)
    foreach(app StretchKitHostTest StretchKitPreview)
        juce_add_console_app(${app} PRODUCT_NAME "${app}")
        target_sources(${app} PRIVATE ${STRETCHKIT_CORE_SOURCES} ${STRETCHKIT_UI_SOURCES})
        target_include_directories(${app} PRIVATE Source)
        target_compile_definitions(${app} PRIVATE
            JucePlugin_Name="StretchKit" JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_FLAC=1 JUCE_USE_OGGVORBIS=1)
        target_link_libraries(${app} PRIVATE
            rubberband StretchKitFonts juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    endforeach()
    target_sources(StretchKitHostTest PRIVATE Tests/HostTest.cpp)
    target_sources(StretchKitPreview PRIVATE Tests/Preview.cpp)
endif()
