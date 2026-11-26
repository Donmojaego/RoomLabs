# =============================================================================
# Room Labs Audio — couche de licence Moonbase commune à tous les plugins.
#
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../common/licensing/RoomLabsLicensing.cmake)
#   rl_add_licensing(MonPlugin ${CMAKE_CURRENT_SOURCE_DIR}/licensing.json)
#
# licensing.json (un par plugin, À COMPLÉTER quand le compte Moonbase est prêt) :
#   enabled            true/false — tant que false, le plugin fonctionne sans licence (version de développement)
#   endpoint           https://<compte>.moonbase.sh
#   productId          identifiant du produit dans Moonbase
#   publicKeyFile      fichier PEM de la CLÉ PUBLIQUE du produit (jamais de clé API secrète dans le plugin)
#   maxDevices         nombre d'ordinateurs par licence — RÉGLÉ DANS MOONBASE, recopié ici pour l'affichage
#   graceDays          jours d'utilisation hors ligne après la dernière vérification en ligne réussie
#   checkIntervalHours délai minimal entre deux vérifications en ligne
#   offline            true : activation hors ligne par fichier proposée
#   trialDays          durée d'essai affichée (0 = pas d'essai)
# =============================================================================
include_guard(GLOBAL)

set(RL_LICENSING_DIR ${CMAKE_CURRENT_LIST_DIR})
set(RL_MOONBASE_TAG "v4.10.0" CACHE STRING "Version de moonbase-cpp (module JUCE moonbase_licensing)")

function(rl_add_licensing target config)
    if(RL_LICENSE_CONFIG_OVERRIDE)                 # essais de compilation avec une configuration de test
        set(config ${RL_LICENSE_CONFIG_OVERRIDE})
    endif()
    file(READ "${config}" _json)
    string(JSON _enabled ERROR_VARIABLE _e GET "${_json}" enabled)
    string(JSON _endpoint ERROR_VARIABLE _e GET "${_json}" endpoint)
    string(JSON _product ERROR_VARIABLE _e GET "${_json}" productId)
    string(JSON _keyfile ERROR_VARIABLE _e GET "${_json}" publicKeyFile)
    string(JSON _maxdev ERROR_VARIABLE _e GET "${_json}" maxDevices)
    string(JSON _grace ERROR_VARIABLE _e GET "${_json}" graceDays)
    string(JSON _interval ERROR_VARIABLE _e GET "${_json}" checkIntervalHours)
    string(JSON _offline ERROR_VARIABLE _e GET "${_json}" offline)
    string(JSON _trial ERROR_VARIABLE _e GET "${_json}" trialDays)
    foreach(v _maxdev _grace _interval _trial)
        if(NOT ${v} MATCHES "^[0-9]+$")
            set(${v} 0)
        endif()
    endforeach()

    get_filename_component(_cfgdir "${config}" DIRECTORY)
    set(_key "")
    if(_keyfile AND EXISTS "${_cfgdir}/${_keyfile}")
        file(READ "${_cfgdir}/${_keyfile}" _key)
    endif()

    set(_on 0)
    if(_enabled STREQUAL "ON" OR _enabled STREQUAL "TRUE" OR _enabled STREQUAL "true")
        if(_endpoint AND _product AND _key)
            set(_on 1)
        else()
            message(WARNING "Licence ${target} : enabled=true mais endpoint, productId ou clé publique manquant — licence désactivée.")
        endif()
    endif()

    # en-tête généré : tous les réglages propres au plugin, rien de secret
    set(_gen ${CMAKE_CURRENT_BINARY_DIR}/rl_license_${target})
    string(REPLACE "\n" "\\n" _keyc "${_key}")
    if(_offline STREQUAL "OFF" OR _offline STREQUAL "FALSE" OR _offline STREQUAL "false")
        set(_offv "false")
    else()
        set(_offv "true")
    endif()
    file(WRITE ${_gen}/rl_license_config.h
"#pragma once\n// Généré par RoomLabsLicensing.cmake depuis licensing.json — ne pas modifier\n#define RL_LICENSING ${_on}\nnamespace rl::licensecfg {\n"
"inline constexpr const char* endpoint = \"${_endpoint}\";\n"
"inline constexpr const char* productId = \"${_product}\";\n"
"inline constexpr const char* publicKey = \"${_keyc}\";\n"
"inline constexpr int maxDevices = ${_maxdev};\n"
"inline constexpr int graceDays = ${_grace};\n"
"inline constexpr int checkIntervalHours = ${_interval};\n"
"inline constexpr bool offline = ${_offv};\n"
"inline constexpr int trialDays = ${_trial};\n}\n")

    target_include_directories(${target} PRIVATE ${_gen} ${RL_LICENSING_DIR})
    target_sources(${target} PRIVATE ${RL_LICENSING_DIR}/RLLicense.cpp)

    # Linux : le module Moonbase passe par libcurl (Windows/macOS : réseau natif)
    if(_on AND UNIX AND NOT APPLE)
        set(RL_USE_CURL 1 PARENT_SCOPE)
    else()
        set(RL_USE_CURL 0 PARENT_SCOPE)
    endif()
    if(_on)
        include(FetchContent)
        FetchContent_Declare(moonbase_cpp
            GIT_REPOSITORY https://github.com/Moonbase-sh/moonbase-cpp.git
            GIT_TAG ${RL_MOONBASE_TAG}
            GIT_SHALLOW TRUE
            SOURCE_SUBDIR modules)              # n'ajoute pas le CMake racine du SDK (OpenSSL/CURL inutiles)
        FetchContent_MakeAvailable(moonbase_cpp)
        if(APPLE)
            enable_language(OBJC OBJCXX)
        endif()
        if(NOT TARGET moonbase_licensing)
            juce_add_module(${moonbase_cpp_SOURCE_DIR}/modules/moonbase_licensing)
        endif()
        target_link_libraries(${target} PRIVATE moonbase_licensing)
        if(UNIX AND NOT APPLE)
            find_package(CURL REQUIRED)
            target_link_libraries(${target} PRIVATE CURL::libcurl)
        endif()
        message(STATUS "Licence ${target} : Moonbase activé (produit ${_product})")
    else()
        message(STATUS "Licence ${target} : non configurée (licensing.json) — version sans protection")
    endif()
endfunction()
