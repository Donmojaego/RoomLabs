cmake_minimum_required(VERSION 3.22)

project(CandyShop VERSION 0.1.0 LANGUAGES C CXX)

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
add_executable(CandyShopEngineTest tests/EngineTest.cpp source/engine/CandyEngine.cpp)
target_include_directories(CandyShopEngineTest PRIVATE source tests)

option(CDY_ENGINE_TEST_ONLY "Ne construire que le test du moteur (sans JUCE)" OFF)
if(CDY_ENGINE_TEST_ONLY)
    return()
endif()

# ---------------------------------------------------------------------------
# JUCE : dossier local (CDY_JUCE_DIR) ou téléchargement automatique
# ---------------------------------------------------------------------------
if(DEFINED CDY_JUCE_DIR)
    add_subdirectory(${CDY_JUCE_DIR} JUCE)
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
juce_add_plugin(CandyShop
    COMPANY_NAME "Room Labs"
    COMPANY_WEBSITE "https://7throomacademy.fr"
    COMPANY_COPYRIGHT "Copyright 2026 Room Labs"
    BUNDLE_ID com.roomlabs.candyshop
    PLUGIN_NAME "Candy Shop"
    PRODUCT_NAME "CandyShop"
    DESCRIPTION "Candy Shop - hooks & party instrument by Room Labs"
    VERSION ${PROJECT_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/icon-1024.png
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rmlb
    PLUGIN_CODE CdyS
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE)

# Interface web (le même dossier que l'aperçu en ligne) + presets d'usine embarqués
file(GLOB CDY_UI_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.webp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/assets/*.svg)
juce_add_binary_data(CandyShopData SOURCES ${CDY_UI_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/presets/factory.json)

target_sources(CandyShop PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/engine/CandyEngine.cpp)
target_include_directories(CandyShop PRIVATE source)

if(MSVC)
    target_compile_options(CandyShop PRIVATE /utf-8)
    target_compile_options(CandyShopEngineTest PRIVATE /utf-8)
endif()

target_compile_definitions(CandyShop PUBLIC
    JUCE_USE_CURL=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    CDY_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(CandyShop
    PRIVATE
        CandyShopData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Test du plugin complet hors DAW (presets, état, MIDI, CPU)
# ---------------------------------------------------------------------------
option(CDY_BUILD_TESTS "Construire le test du plugin" OFF)
if(CDY_BUILD_TESTS)
    juce_add_console_app(CandyShopRenderTest PRODUCT_NAME "CandyShopRenderTest")
    target_sources(CandyShopRenderTest PRIVATE
        tests/RenderTest.cpp
        source/PluginProcessor.cpp
        source/engine/CandyEngine.cpp)
    target_include_directories(CandyShopRenderTest PRIVATE source)
    target_compile_definitions(CandyShopRenderTest PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 CDY_HEADLESS=1
        CDY_VERSION_STRING="${PROJECT_VERSION}" JucePlugin_Name="Candy Shop")
    if(MSVC)
        target_compile_options(CandyShopRenderTest PRIVATE /utf-8)
    endif()
    target_link_libraries(CandyShopRenderTest PRIVATE
        CandyShopData juce::juce_audio_processors juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
