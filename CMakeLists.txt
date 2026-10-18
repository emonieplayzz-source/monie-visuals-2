cmake_minimum_required(VERSION 3.22)
project(MonieVisuals VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

option(MONIE_BUILD_TESTS "Build the analyzer unit test" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE stb)

juce_add_plugin(MonieVisuals
    COMPANY_NAME "monie"
    PRODUCT_NAME "monie visuals"
    PLUGIN_MANUFACTURER_CODE Mnvs
    PLUGIN_CODE Mvis
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(MonieShaders SOURCES
    Shaders/pass.vert Shaders/scene.frag Shaders/post.frag)

target_sources(MonieVisuals PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VisualizerComponent.cpp
    Source/ImageLibrary.cpp)

target_include_directories(MonieVisuals PRIVATE Source Source/Core ${stb_SOURCE_DIR})

target_compile_definitions(MonieVisuals PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(MonieVisuals
    PRIVATE MonieShaders juce::juce_audio_utils juce::juce_audio_processors juce::juce_opengl
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

if(MONIE_BUILD_TESTS)
    add_executable(analyzer_test tests/test_analyzer.cpp)
    target_include_directories(analyzer_test PRIVATE Source/Core)
    add_executable(engines_test tests/test_engines.cpp)
    target_include_directories(engines_test PRIVATE Source/Core)
endif()
