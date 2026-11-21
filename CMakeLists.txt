cmake_minimum_required(VERSION 3.22)
project(CrowdBase VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/JUCE)
juce_add_binary_data(CrowdAssets SOURCES resources/phonemes.wav
 resources/generated/ivory.png resources/generated/graphite.png resources/generated/walnut.png
 resources/generated/knob.png resources/generated/meter.png resources/generated/switches.png
 resources/generated/buttons.png resources/generated/lamps.png resources/generated/vfd.png
 resources/generated/transport.png resources/generated/app-icon.png)
juce_add_plugin(CrowdBase COMPANY_NAME "Lakam Audio" BUNDLE_ID "audio.lakam.crowdbase"
 PLUGIN_MANUFACTURER_CODE Lkam PLUGIN_CODE Crdb FORMATS VST3 PRODUCT_NAME "CrowdBase"
 IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE COPY_PLUGIN_AFTER_BUILD FALSE
 VST3_CATEGORIES Instrument Generator NEEDS_WEB_BROWSER FALSE NEEDS_CURL FALSE
 ICON_BIG resources/generated/app-icon.png ICON_SMALL resources/generated/app-icon.png)
target_sources(CrowdBase PRIVATE Source/Engine.cpp Source/Processor.cpp Source/Editor.cpp
 Source/RasterAssets.cpp Source/VfdDisplay.cpp)
target_include_directories(CrowdBase PUBLIC Source "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/modules")
target_compile_definitions(CrowdBase PUBLIC JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0 JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(CrowdBase PRIVATE CrowdAssets juce::juce_audio_utils juce::juce_dsp
 PUBLIC juce::juce_recommended_config_flags)
juce_add_gui_app(CrowdBaseStandalone COMPANY_NAME "Lakam Audio" PRODUCT_NAME "CrowdBase"
 NEEDS_WEB_BROWSER FALSE NEEDS_CURL FALSE
 ICON_BIG resources/generated/app-icon.png ICON_SMALL resources/generated/app-icon.png)
target_sources(CrowdBaseStandalone PRIVATE Source/Standalone.cpp Source/Tests.cpp)
target_link_libraries(CrowdBaseStandalone PRIVATE CrowdBase juce::juce_recommended_config_flags)
if(WIN32)
 target_link_libraries(CrowdBaseStandalone PRIVATE dwmapi)
endif()
