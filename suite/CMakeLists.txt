cmake_minimum_required(VERSION 3.22)
# =============================================================================
# Room Labs Audio — suite de plugins (un CMake pour tous ; chaque plugin s'ajoute via rl_add_plugin)
# Version de la suite = version des installeurs RoomLabsAudio_Plugins_<version>_Win_x64.exe
# =============================================================================
project(RoomLabsAudioSuite VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS minimum")
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()
if(MSVC)
    add_compile_options(/utf-8 $<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# ---------------------------------------------------------------------------
# Tests DSP sans JUCE
# ---------------------------------------------------------------------------
add_executable(NoiseTest tests/NoiseTest.cpp plugins/noise/NoiseDSP.cpp)
add_executable(A2MTest tests/A2MTest.cpp plugins/audio_to_midi/A2MDSP.cpp)
add_executable(RackTest tests/RackTest.cpp plugins/rack/RackDSP.cpp plugins/noise/NoiseDSP.cpp plugins/reverb/ReverbDSP.cpp plugins/loop_remix_lab/RemixDSP.cpp)
add_executable(RemixTest tests/RemixTest.cpp plugins/loop_remix_lab/RemixDSP.cpp)
add_executable(ReverbTest tests/ReverbTest.cpp plugins/reverb/ReverbDSP.cpp)
add_executable(NoiseToolsTest tests/NoiseToolsTest.cpp plugins/noise_tools/NoiseToolsDSP.cpp)
target_include_directories(NoiseToolsTest PRIVATE plugins/noise_tools)

option(RL_DSP_TEST_ONLY "Ne construire que les tests DSP" OFF)
if(RL_DSP_TEST_ONLY)
    return()
endif()

if(DEFINED RL_JUCE_DIR)
    add_subdirectory(${RL_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.15 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/RoomLabsLicensing.cmake)

# ---------------------------------------------------------------------------
# rl_add_plugin(<Cible> NAME <nom> CODE <4 lettres> PRODUCT <vst/dossier> FACE <w> <h>
#               SOURCES ... [CATEGORIES ...] [SYNTH])
# Interface : ui/ commun + ui/ du plugin + face/cadres du produit + sprites partagés + meta.json
# ---------------------------------------------------------------------------
function(rl_add_plugin target)
    cmake_parse_arguments(A "SYNTH;MIDIOUT" "NAME;CODE;PRODUCT;DIR" "FACE;SOURCES;CATEGORIES" ${ARGN})
    set(au_type kAudioUnitType_Effect)
    if(A_MIDIOUT)
        set(au_type kAudioUnitType_MusicEffect)
    else()
        set(A_MIDIOUT FALSE)
    endif()
    juce_add_plugin(${target}
        COMPANY_NAME "Room Labs Audio"
        COMPANY_WEBSITE "https://7throomacademy.fr"
        COMPANY_COPYRIGHT "Copyright 2026 Room Labs Audio"
        BUNDLE_ID com.roomlabs.${A_CODE}
        PLUGIN_NAME "${A_NAME}"
        PRODUCT_NAME "${A_NAME}"
        DESCRIPTION "${A_NAME} - Room Labs Audio"
        VERSION ${PROJECT_VERSION}
        IS_SYNTH ${A_SYNTH}
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT ${A_MIDIOUT}
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE Rmlb
        PLUGIN_CODE ${A_CODE}
        FORMATS VST3 AU Standalone
        VST3_CATEGORIES ${A_CATEGORIES}
        AU_MAIN_TYPE ${au_type}
        NEEDS_WEB_BROWSER TRUE
        NEEDS_WEBVIEW2 TRUE)
    get_filename_component(pdir ${A_PRODUCT} NAME)
    file(GLOB ui CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/common/ui/*
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/${A_DIR}/ui/*
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/${A_PRODUCT}/*.webp
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/_commandes/*.webp
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/meta.json
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/rl_license_ui.js)
    juce_add_binary_data(${target}Data SOURCES ${ui})
    target_sources(${target} PRIVATE common/RLCommon.cpp ${A_SOURCES})
    rl_add_licensing(${target} ${CMAKE_CURRENT_SOURCE_DIR}/plugins/${A_DIR}/licensing.json)
    target_include_directories(${target} PRIVATE common plugins/${A_DIR})
    list(GET A_FACE 0 fw)
    list(GET A_FACE 1 fh)
    target_compile_definitions(${target} PUBLIC
        JUCE_USE_CURL=${RL_USE_CURL} JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0 RL_FACE_W=${fw} RL_FACE_H=${fh})
    target_link_libraries(${target}
        PRIVATE ${target}Data juce::juce_audio_utils juce::juce_gui_extra
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

# ---------------------------------------------------------------------------
# Plugins (la liste fait foi avec manifest.json)
# ---------------------------------------------------------------------------
rl_add_plugin(NoiseTools NAME "Noise Tools" CODE NsTl PRODUCT vst/noise_tools DIR noise_tools FACE 754 545
    SOURCES plugins/noise_tools/NoiseToolsProcessor.cpp plugins/noise_tools/NoiseToolsDSP.cpp
    CATEGORIES Fx Restoration)

rl_add_plugin(Noise NAME "Noise" CODE Nois PRODUCT vst/noise DIR noise FACE 660 515
    SOURCES plugins/noise/NoiseProcessor.cpp plugins/noise/NoiseDSP.cpp
    CATEGORIES Fx Generator)

rl_add_plugin(Reverb NAME "Reverb" CODE Rvrb PRODUCT vst/reverb DIR reverb FACE 740 640
    SOURCES plugins/reverb/ReverbProcessor.cpp plugins/reverb/ReverbDSP.cpp
    CATEGORIES Fx Reverb)

rl_add_plugin(LoopRemixLab NAME "Loop Remix Lab" CODE LpRx PRODUCT vst/loop_remix_lab DIR loop_remix_lab FACE 880 545
    SOURCES plugins/loop_remix_lab/RemixProcessor.cpp plugins/loop_remix_lab/RemixDSP.cpp
    CATEGORIES Fx)

rl_add_plugin(AudioToMidi NAME "Audio to MIDI" CODE A2Md PRODUCT vst/audio_to_midi DIR audio_to_midi FACE 950 530 MIDIOUT
    SOURCES plugins/audio_to_midi/A2MProcessor.cpp plugins/audio_to_midi/A2MDSP.cpp
    CATEGORIES Fx Analyzer)

rl_add_plugin(SevenRoomRack NAME "7 Room Rack" CODE Rk7R PRODUCT rack/chassis DIR rack FACE 950 630
    SOURCES plugins/rack/RackProcessor.cpp plugins/rack/RackDSP.cpp plugins/noise/NoiseDSP.cpp plugins/reverb/ReverbDSP.cpp plugins/loop_remix_lab/RemixDSP.cpp
    CATEGORIES Fx)
