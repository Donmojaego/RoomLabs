cmake_minimum_required(VERSION 3.22)

project(ShakeIt VERSION 0.1.0 LANGUAGES C CXX)

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
add_executable(ShakeItEngineTest tests/EngineTest.cpp source/engine/BassEngine.cpp)
target_include_directories(ShakeItEngineTest PRIVATE source tests)

option(SHK_ENGINE_TEST_ONLY "Ne construire que le test du moteur (sans JUCE)" OFF)
if(SHK_ENGINE_TEST_ONLY)
    return()
endif()

# ---------------------------------------------------------------------------
# JUCE : dossier local (SHK_JUCE_DIR) ou téléchargement automatique
# ---------------------------------------------------------------------------
if(DEFINED SHK_JUCE_DIR)
    add_subdirectory(${SHK_JUCE_DIR} JUCE)
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
juce_add_plugin(ShakeIt
    COMPANY_NAME "Room Labs"
    COMPANY_WEBSITE "https://7throomacademy.fr"
    COMPANY_COPYRIGHT "Copyright 2026 Room Labs"
    BUNDLE_ID com.roomlabs.shakeit
    PLUGIN_NAME "Shake It"
    PRODUCT_NAME "ShakeIt"
    DESCRIPTION "Shake It - 808 & bass instrument by Room Labs"
    VERSION ${PROJECT_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/icon-1024.png
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rmlb
    PLUGIN_CODE ShkI
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE)

# Interface web (le même dossier que l'aperçu en ligne) + presets d'usine embarqués
file(GLOB SHK_UI_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.webp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.svg)
juce_add_binary_data(ShakeItData SOURCES ${SHK_UI_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/presets/factory.json)

target_sources(ShakeIt PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/engine/BassEngine.cpp)
target_include_directories(ShakeIt PRIVATE source)

if(MSVC)
    target_compile_options(ShakeIt PRIVATE /utf-8)
    target_compile_options(ShakeItEngineTest PRIVATE /utf-8)
endif()

target_compile_definitions(ShakeIt PUBLIC
    JUCE_USE_CURL=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    SHK_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(ShakeIt
    PRIVATE
        ShakeItData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Test du plugin complet hors DAW (presets, état, MIDI, CPU)
# ---------------------------------------------------------------------------
option(SHK_BUILD_TESTS "Construire le test du plugin" OFF)
if(SHK_BUILD_TESTS)
    juce_add_console_app(ShakeItRenderTest PRODUCT_NAME "ShakeItRenderTest")
    target_sources(ShakeItRenderTest PRIVATE
        tests/RenderTest.cpp
        source/PluginProcessor.cpp
        source/engine/BassEngine.cpp)
    target_include_directories(ShakeItRenderTest PRIVATE source)
    target_compile_definitions(ShakeItRenderTest PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 SHK_HEADLESS=1
        SHK_VERSION_STRING="${PROJECT_VERSION}" JucePlugin_Name="Shake It")
    if(MSVC)
        target_compile_options(ShakeItRenderTest PRIVATE /utf-8)
    endif()
    target_link_libraries(ShakeItRenderTest PRIVATE
        ShakeItData juce::juce_audio_processors juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
