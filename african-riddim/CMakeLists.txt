cmake_minimum_required(VERSION 3.22)

project(AfricanRiddim VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS minimum")
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()

# Visual Studio : symboles de débogage (.pdb) aussi en Release, livrés à part
if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# ---------------------------------------------------------------------------
# Test du moteur, sans JUCE (rapide, tourne partout)
# ---------------------------------------------------------------------------
add_executable(AfricanEngineTest tests/EngineTest.cpp source/engine/AfricanEngine.cpp)
target_include_directories(AfricanEngineTest PRIVATE source source/engine tests)

option(AFR_ENGINE_TEST_ONLY "Ne construire que le test du moteur (sans JUCE)" OFF)
if(AFR_ENGINE_TEST_ONLY)
    return()
endif()

# ---------------------------------------------------------------------------
# JUCE : dossier local (AFR_JUCE_DIR) ou téléchargement automatique
# ---------------------------------------------------------------------------
if(DEFINED AFR_JUCE_DIR)
    add_subdirectory(${AFR_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(AfricanRiddim
    COMPANY_NAME "Room Labs Audio"
    COMPANY_WEBSITE "https://roomlabsaudio.com/plugins"
    COMPANY_COPYRIGHT "(C) 2026 Room Labs Audio - (P) 2026 Room Labs Audio"
    BUNDLE_ID com.roomlabs.africanriddim
    PLUGIN_NAME "African Riddim"
    PRODUCT_NAME "African Riddim"
    DESCRIPTION "African Riddim - 54 instruments d Afrique, par Room Labs"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rmlb
    PLUGIN_CODE AfRd
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE)

# Interface web (le même dossier que l'aperçu en ligne) + presets d'usine embarqués
file(GLOB AFR_UI_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.webp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/data.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/rl_license_ui.js)
juce_add_binary_data(AfricanData SOURCES ${AFR_UI_FILES})

include(${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/RoomLabsLicensing.cmake)
rl_add_licensing(AfricanRiddim ${CMAKE_CURRENT_SOURCE_DIR}/licensing.json)

target_sources(AfricanRiddim PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/engine/AfricanEngine.cpp)
target_include_directories(AfricanRiddim PRIVATE source source/engine)

if(MSVC)
    target_compile_options(AfricanRiddim PRIVATE /utf-8)
    target_compile_options(AfricanEngineTest PRIVATE /utf-8)
endif()

target_compile_definitions(AfricanRiddim PUBLIC
    JUCE_USE_CURL=${RL_USE_CURL}
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    AFR_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(AfricanRiddim
    PRIVATE
        AfricanData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Test du plugin complet hors DAW (presets, état, MIDI, CPU)
# ---------------------------------------------------------------------------
option(AFR_BUILD_TESTS "Construire le test du plugin" OFF)
if(AFR_BUILD_TESTS)
    juce_add_console_app(AfricanRenderTest PRODUCT_NAME "AfricanRenderTest")
    target_sources(AfricanRenderTest PRIVATE
        tests/RenderTest.cpp
        source/PluginProcessor.cpp
        source/engine/AfricanEngine.cpp)
    target_include_directories(AfricanRenderTest PRIVATE source source/engine)
    rl_add_licensing(AfricanRenderTest ${CMAKE_CURRENT_SOURCE_DIR}/licensing.json)
    target_compile_definitions(AfricanRenderTest PRIVATE JUCE_USE_CURL=${RL_USE_CURL} JUCE_WEB_BROWSER=0 AFR_HEADLESS=1
        AFR_VERSION_STRING="${PROJECT_VERSION}" JucePlugin_Name="African Riddim")
    if(MSVC)
        target_compile_options(AfricanRenderTest PRIVATE /utf-8)
    endif()
    target_link_libraries(AfricanRenderTest PRIVATE
        AfricanData juce::juce_audio_processors juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
