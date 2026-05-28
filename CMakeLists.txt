cmake_minimum_required(VERSION 3.16)
project(VideoPlayer VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CcpBuildConfigurations)
include(CcpTargetConfigurations)
include(CcpPackageConfigHelpers)
include(CcpMonolithVendorConfig)

find_package(Vorbis CONFIG REQUIRED)
find_package(libvpx REQUIRED)
find_package(libnestegg CONFIG REQUIRED)

find_package(carbon-blue CONFIG REQUIRED)
find_package(carbon-blueexposure CONFIG REQUIRED)
find_package(carbon-trinityaudioapi CONFIG REQUIRED)


find_package(PNG REQUIRED)

if(WIN32)
    find_package(carbon-trinity REQUIRED CONFIG COMPONENTS Trinity_stub)
elseif(APPLE)
    find_package(carbon-trinity REQUIRED CONFIG COMPONENTS Trinity_metal)
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

find_package(GTest REQUIRED)

ccp_add_library(VideoPlayer SHARED
    FrameQueue.h
    IAudioDecoder.h
    IAudioSink.h
    IAudioSinkExposed.h
    IVideoContainerParser.h
    IVideoDecoder.h
    Metadata.h
    StdAfx.h
    Timer.h
    VideoController.h
    VideoPlayer.h
    VideoPlayerResult.h
    VorbisDecoder.h
    VpxDecoder.h
    WaveOutAudioSink.h
    WebMParser.h
    WwiseAudioSink.h
    IAudioDecoder.cpp
    IAudioSink.cpp
    IVideoContainerParser.cpp
    IVideoDecoder.cpp
    Metadata.cpp
    StdAfx.cpp
    Timer.cpp
    VideoController.cpp
    VideoPlayer.cpp
    VideoPlayerResult.cpp
    VideoPlayerResult_Blue.cpp
    VideoPlayer_Blue.cpp
    VorbisDecoder.cpp
    VpxDecoder.cpp
    WaveOutAudioSink.cpp
    WaveOutAudioSink_Blue.cpp
    WebMParser.cpp
    WwiseAudioSink.cpp
    WwiseAudioSink_Blue.cpp
)

target_include_directories(VideoPlayer PUBLIC "$<TARGET_PROPERTY:CcpTrinityAudioAPI,INTERFACE_INCLUDE_DIRECTORIES>")
if(WIN32)
    target_include_directories(VideoPlayer PRIVATE "$<TARGET_PROPERTY:CcpTrinity_stub,INTERFACE_INCLUDE_DIRECTORIES>")
elseif(APPLE)
    target_include_directories(VideoPlayer PRIVATE "$<TARGET_PROPERTY:CcpTrinity_metal,INTERFACE_INCLUDE_DIRECTORIES>")
endif()
target_link_libraries(VideoPlayer PRIVATE Blue Vorbis::vorbis WebM::vpx nestegg)

if(WIN32)
    target_link_libraries(VideoPlayer PRIVATE Winmm)
endif()

target_precompile_headers(VideoPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_include_directories(VideoPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(VideoPlayer PUBLIC "${MATH_OPTIMIZE_FLAG}")

add_trinity_dev_debug_flags(VideoPlayer)

if(APPLE)
    set_target_properties(
        VideoPlayer
        PROPERTIES
            PREFIX ""
            SUFFIX ".so"
    )
endif()
set_target_properties(VideoPlayer PROPERTIES OUTPUT_NAME "_videoplayer")
set_target_properties(VideoPlayer PROPERTIES CXX_VISIBILITY_PRESET hidden)

get_target_property(_SOURCES VideoPlayer SOURCES)

# Provide an install target if this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)

    if(INSTALL_TO_MONOLITH)
        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
            TARGETS VideoPlayer
            EXPORT VideoPlayerTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )

        configure_ccp_vendor_config_file(
            TARGET VideoPlayer
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/VideoPlayerConfig.cmake"
            DEPENDENCIES ""
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VideoPlayerConfig.cmake DESTINATION .)

    else()
        install(
            TARGETS VideoPlayer
            EXPORT VideoPlayerTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/
            ARCHIVE DESTINATION lib/
            RUNTIME DESTINATION bin/
        )

        install(
            EXPORT VideoPlayerTargets
            DESTINATION share/carbon-videoplayer
            FILE carbon-videoplayerConfig.cmake
        )
    endif()

     # Install python package
     install(DIRECTORY python/ DESTINATION python
        FILES_MATCHING
        PATTERN "*.py"
     )
endif()


ccp_add_executable(VideoPlayerTest
    ${_SOURCES}
    tests/TestUtilities.h
    tests/TestFrameQueue.cpp
    tests/TestUtilities.cpp
    tests/TestVideoPlayerController.cpp
    tests/TestVorbisDecoder.cpp
    tests/TestVpxDecoder.cpp
    tests/TestWebMParser.cpp
    tests/main.cpp
)

if(CMAKE_IMPORT_LIBRARY_SUFFIX)
    add_custom_command(TARGET VideoPlayerTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:VideoPlayerTest> $<TARGET_FILE_DIR:VideoPlayerTest>
            COMMAND_EXPAND_LISTS
            VERBATIM
    )
else()
    add_custom_command(TARGET VideoPlayerTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_LINKER_FILE:Python> $<TARGET_FILE_DIR:VideoPlayerTest>
            COMMAND_EXPAND_LISTS
            VERBATIM
    )
endif()

if(WIN32)
    target_include_directories(VideoPlayerTest PRIVATE "$<TARGET_PROPERTY:CcpTrinity_stub,INTERFACE_INCLUDE_DIRECTORIES>")
elseif(APPLE)
    target_include_directories(VideoPlayerTest PRIVATE "$<TARGET_PROPERTY:CcpTrinity_metal,INTERFACE_INCLUDE_DIRECTORIES>")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

target_include_directories(VideoPlayerTest PUBLIC "$<TARGET_PROPERTY:Blue,INTERFACE_INCLUDE_DIRECTORIES>")


target_link_libraries(VideoPlayerTest PRIVATE Vorbis::vorbis WebM::vpx nestegg GTest::GTest GTest::Main BlueExposure)

get_target_property(_SOURCES VideoPlayerTest SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

target_precompile_headers(VideoPlayerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_compile_definitions(VideoPlayerTest PRIVATE WITH_TESTS)
target_include_directories(VideoPlayerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(VideoPlayerTest PRIVATE BLUEBUILD)

if(APPLE)
    target_link_libraries(VideoPlayerTest PRIVATE "-framework CoreFoundation -framework Cocoa")
elseif(WIN32)
    target_link_libraries(VideoPlayerTest PRIVATE Winmm)
endif()

set_target_properties(VideoPlayerTest PROPERTIES FOLDER "Tests")
