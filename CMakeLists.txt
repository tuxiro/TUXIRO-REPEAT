cmake_minimum_required(VERSION 3.22)

# macOS: binario universal (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Min macOS" FORCE)
endif()

# Windows: runtime estatico (no hace falta instalar Visual C++ Redistributable)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(TUXIRO_REPEAT VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

option(TUXIRO_COPY_AFTER_BUILD "Copiar el VST3 a la carpeta de plugins tras compilar" OFF)

juce_add_plugin(TuxiroRepeat
    COMPANY_NAME "Tuxiro"
    PLUGIN_MANUFACTURER_CODE Tuxi
    PLUGIN_CODE Trpt
    FORMATS VST3
    PRODUCT_NAME "TUXIRO REPEAT"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${TUXIRO_COPY_AFTER_BUILD})

target_sources(TuxiroRepeat PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(TuxiroRepeat PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TuxiroRepeat
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
