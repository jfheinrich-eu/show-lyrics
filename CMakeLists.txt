cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a checked-in CMake preset.")
endif()

project(
    ShowLyrics
    VERSION 0.1.0
    LANGUAGES C CXX
)

option(SHOW_LYRICS_BUILD_PLUGIN "Build the Show Lyrics VST3 plug-in" ON)

include(CTest)
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 72782788ce18c2d4d760b28e0921d6ffc6431102 # 9.0.2
)

FetchContent_MakeAvailable(JUCE)

add_library(
    ShowLyricsDomain
    STATIC
        src/model/CueGenerationExchange.cpp
        src/model/CueGenerationExchange.h
        src/model/CuePlaybackIndex.cpp
        src/model/CuePlaybackIndex.h
        src/model/CueState.cpp
        src/model/CueState.h
        src/model/CueStateLimits.h
        src/model/CueStateSerializer.cpp
        src/model/CueStateSerializer.h
        src/model/LyricCue.cpp
        src/model/LyricCue.h
        src/model/TransportSnapshot.h
        src/model/TransportSnapshotMailbox.cpp
        src/model/TransportSnapshotMailbox.h
)

target_include_directories(
    ShowLyricsDomain
    PUBLIC
        src
)

target_compile_features(ShowLyricsDomain PUBLIC cxx_std_17)

target_compile_definitions(ShowLyricsDomain PUBLIC JUCE_USE_CURL=0)

target_link_libraries(
    ShowLyricsDomain
    PUBLIC
        juce::juce_core
        juce::juce_data_structures
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

if(SHOW_LYRICS_BUILD_PLUGIN)
    juce_add_plugin(
        ShowLyricsPlugin
        COMPANY_NAME "jfheinrich"
        BUNDLE_ID "eu.jfheinrich.showlyrics"
        PLUGIN_MANUFACTURER_CODE Jfhe
        PLUGIN_CODE ShLy
        FORMATS VST3
        PRODUCT_NAME "Show Lyrics"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE
    )

    target_sources(
        ShowLyricsPlugin
        PRIVATE
            src/plugin/PluginEditor.cpp
            src/plugin/PluginEditor.h
            src/plugin/PluginProcessor.cpp
            src/plugin/PluginProcessor.h
    )

    target_compile_features(ShowLyricsPlugin PUBLIC cxx_std_17)

    target_compile_definitions(
        ShowLyricsPlugin
        PUBLIC
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_WEB_BROWSER=0
    )

    target_link_libraries(
        ShowLyricsPlugin
        PRIVATE
            ShowLyricsDomain
            juce::juce_audio_utils
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endif()

if(BUILD_TESTING)
    add_executable(
        ShowLyricsDomainTests
        tests/LyricCueTests.cpp
    )

    add_executable(
        ShowLyricsCuePlaybackIndexTests
        tests/CuePlaybackIndexTests.cpp
    )

    add_executable(
        ShowLyricsRealtimeCommunicationTests
        tests/RealtimeCommunicationTests.cpp
    )

    target_compile_features(ShowLyricsDomainTests PRIVATE cxx_std_17)
    target_compile_features(ShowLyricsCuePlaybackIndexTests PRIVATE cxx_std_17)
    target_compile_features(ShowLyricsRealtimeCommunicationTests PRIVATE cxx_std_17)
    target_link_libraries(ShowLyricsDomainTests PRIVATE ShowLyricsDomain)
    target_link_libraries(ShowLyricsCuePlaybackIndexTests PRIVATE ShowLyricsDomain)
    target_link_libraries(ShowLyricsRealtimeCommunicationTests PRIVATE ShowLyricsDomain)

    add_test(NAME ShowLyricsDomainTests COMMAND ShowLyricsDomainTests)
    add_test(NAME ShowLyricsCuePlaybackIndexTests COMMAND ShowLyricsCuePlaybackIndexTests)
    add_test(NAME ShowLyricsRealtimeCommunicationTests COMMAND ShowLyricsRealtimeCommunicationTests)
endif()

if(MSVC)
    target_compile_options(ShowLyricsDomain PRIVATE /permissive- /W4)
    if(SHOW_LYRICS_BUILD_PLUGIN)
        target_compile_options(ShowLyricsPlugin PRIVATE /permissive- /W4)
    endif()
    if(BUILD_TESTING)
        target_compile_options(ShowLyricsDomainTests PRIVATE /permissive- /W4)
        target_compile_options(ShowLyricsCuePlaybackIndexTests PRIVATE /permissive- /W4)
        target_compile_options(ShowLyricsRealtimeCommunicationTests PRIVATE /permissive- /W4)
    endif()
endif()

include(cmake/Packaging.cmake)
