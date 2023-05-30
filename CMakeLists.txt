cmake_minimum_required(VERSION 3.16)
project(VideoPlayer VERSION 1.0.0)

if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)

find_package(vorbis REQUIRED)
find_package(vpx REQUIRED)
find_package(nestegg REQUIRED)

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

find_package(PNG REQUIRED NO_CMAKE_PATH)
find_package(Blue REQUIRED NO_CMAKE_PATH)
find_package(CarbonAudio REQUIRED NO_CMAKE_PATH)
target_link_libraries(VideoPlayer PRIVATE Blue VORBIS VPX NestEgg CarbonAudio)

if(WIN32)
    find_package(Trinity_stub REQUIRED NO_CMAKE_PATH)
elseif(APPLE)
    find_package(Trinity_metal REQUIRED NO_CMAKE_PATH)
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

target_include_directories(VideoPlayer PUBLIC "$<$<BOOL:$<TARGET_EXISTS:trinity_stub>>:$<TARGET_PROPERTY:trinity_stub,INTERFACE_INCLUDE_DIRECTORIES>>")
target_include_directories(VideoPlayer PUBLIC "$<$<BOOL:$<TARGET_EXISTS:trinity_metal>>:$<TARGET_PROPERTY:trinity_metal,INTERFACE_INCLUDE_DIRECTORIES>>")


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

target_include_directories(VideoPlayerTest PUBLIC "$<$<BOOL:$<TARGET_EXISTS:trinity_stub>>:$<TARGET_PROPERTY:trinity_stub,INTERFACE_INCLUDE_DIRECTORIES>>")
target_include_directories(VideoPlayerTest PUBLIC "$<$<BOOL:$<TARGET_EXISTS:trinity_metal>>:$<TARGET_PROPERTY:trinity_metal,INTERFACE_INCLUDE_DIRECTORIES>>")

find_package(GTest REQUIRED)

target_include_directories(VideoPlayerTest PUBLIC "$<TARGET_PROPERTY:Blue,INTERFACE_INCLUDE_DIRECTORIES>")
target_include_directories(VideoPlayerTest PUBLIC "$<TARGET_PROPERTY:CarbonAudio,INTERFACE_INCLUDE_DIRECTORIES>")


target_link_libraries(VideoPlayerTest PRIVATE VORBIS VPX NestEgg GTest::GTest GTest::Main BlueExposure)

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
