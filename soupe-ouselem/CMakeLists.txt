cmake_minimum_required(VERSION 3.22)

project(SoupeOuselem VERSION 0.1.1 LANGUAGES C CXX)

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
add_executable(SoupeOuselemEngineTest tests/EngineTest.cpp source/engine/SoupEngine.cpp)
target_include_directories(SoupeOuselemEngineTest PRIVATE source tests)

option(SOUP_ENGINE_TEST_ONLY "Ne construire que le test du moteur (sans JUCE)" OFF)
if(SOUP_ENGINE_TEST_ONLY)
    return()
endif()

# ---------------------------------------------------------------------------
# JUCE : dossier local (SOUP_JUCE_DIR) ou téléchargement automatique
# ---------------------------------------------------------------------------
if(DEFINED SOUP_JUCE_DIR)
    add_subdirectory(${SOUP_JUCE_DIR} JUCE)
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
juce_add_plugin(SoupeOuselem
    COMPANY_NAME "Room Labs"
    COMPANY_WEBSITE "https://roomlabsaudio.com/plugins"
    COMPANY_COPYRIGHT "(C) 2026 Room Labs Audio - (P) 2026 Room Labs Audio"
    BUNDLE_ID com.roomlabs.soupeouselem
    PLUGIN_NAME "Ma petite soupe ouselem"
    PRODUCT_NAME "SoupeOuselem"
    DESCRIPTION "Ma petite soupe ouselem - instrument + synced FX by Room Labs"
    VERSION ${PROJECT_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/icon-1024.png
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rmlb
    PLUGIN_CODE SpOu
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE)

# Interface web (le même dossier que l'aperçu en ligne) + presets d'usine embarqués
file(GLOB SOUP_UI_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.webp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/rl_license_ui.js
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/layout.json)
juce_add_binary_data(SoupeOuselemData SOURCES ${SOUP_UI_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/presets/factory.json)

# licence Moonbase (réglages propres au plugin : licensing.json)
include(${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/RoomLabsLicensing.cmake)
rl_add_licensing(SoupeOuselem ${CMAKE_CURRENT_SOURCE_DIR}/licensing.json)

target_sources(SoupeOuselem PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/engine/SoupEngine.cpp)
target_include_directories(SoupeOuselem PRIVATE source)

if(MSVC)
    target_compile_options(SoupeOuselem PRIVATE /utf-8)
    target_compile_options(SoupeOuselemEngineTest PRIVATE /utf-8)
endif()

target_compile_definitions(SoupeOuselem PUBLIC
    JUCE_USE_CURL=${RL_USE_CURL}
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    SOUP_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(SoupeOuselem
    PRIVATE
        SoupeOuselemData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Test du plugin complet hors DAW (presets, état, MIDI, CPU)
# ---------------------------------------------------------------------------
option(SOUP_BUILD_TESTS "Construire le test du plugin" OFF)
if(SOUP_BUILD_TESTS)
    juce_add_console_app(SoupeOuselemRenderTest PRODUCT_NAME "SoupeOuselemRenderTest")
    target_sources(SoupeOuselemRenderTest PRIVATE
        tests/RenderTest.cpp
        source/PluginProcessor.cpp
        source/engine/SoupEngine.cpp)
    target_include_directories(SoupeOuselemRenderTest PRIVATE source)
    rl_add_licensing(SoupeOuselemRenderTest ${CMAKE_CURRENT_SOURCE_DIR}/licensing.json)
    target_compile_definitions(SoupeOuselemRenderTest PRIVATE JUCE_USE_CURL=${RL_USE_CURL} JUCE_WEB_BROWSER=0 SOUP_HEADLESS=1
        SOUP_VERSION_STRING="${PROJECT_VERSION}" JucePlugin_Name="Ma petite soupe ouselem")
    if(MSVC)
        target_compile_options(SoupeOuselemRenderTest PRIVATE /utf-8)
    endif()
    target_link_libraries(SoupeOuselemRenderTest PRIVATE
        SoupeOuselemData juce::juce_audio_processors juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
