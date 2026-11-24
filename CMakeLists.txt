cmake_minimum_required(VERSION 3.22)
# CRT estático en Windows: no hace falta instalar el runtime de Visual C++
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(JustAnAmp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# JUCE: usa una copia local si se pasa -DJUCE_LOCAL_PATH=..., si no la descarga.
if(DEFINED JUCE_LOCAL_PATH)
  add_subdirectory(${JUCE_LOCAL_PATH} juce EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(JustAnAmpAssets SOURCES
  Assets/amp_pulga.jpg
  Assets/amp_notfieldy.jpg
  Assets/amp_roagu.jpg
  Assets/amp_ryan.jpg
  Assets/amp_canciller.jpg
  Assets/amp_timmyc.jpg)

juce_add_plugin(JustAnAmp
  COMPANY_NAME "JustAnAmp"
  PLUGIN_MANUFACTURER_CODE Jaam
  PLUGIN_CODE Jaab
  FORMATS VST3
  PRODUCT_NAME "JustAnAmp"
  VST3_CATEGORIES Fx
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(JustAnAmp PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(JustAnAmp PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(JustAnAmp
  PRIVATE
    JustAnAmpAssets
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
