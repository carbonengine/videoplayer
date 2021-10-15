project(VideoPlayer VERSION 1.0.0)

find_package(vorbis REQUIRED)
find_package(vpx REQUIRED)
find_package(nestegg REQUIRED)

add_library(VideoPlayer SHARED
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
target_link_libraries(VideoPlayer PRIVATE Blue VORBIS VPX NestEgg)

if(WIN32)
    target_link_libraries(VideoPlayer PRIVATE Winmm)
endif()

target_precompile_headers(VideoPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_include_directories(VideoPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(VideoPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

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

add_executable(VideoPlayerTest
    ${_SOURCES}
    test/TestUtilities.h
    test/TestFrameQueue.cpp
    test/TestUtilities.cpp
    test/TestVideoPlayerController.cpp
    test/TestVorbisDecoder.cpp
    test/TestVpxDecoder.cpp
    test/TestWebMParser.cpp
    test/main.cpp
)
target_link_libraries(VideoPlayerTest PRIVATE VORBIS VPX NestEgg GTest::GTest GTest::Main BlueExposure)

get_target_property(_SOURCES VideoPlayerTest SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

target_precompile_headers(VideoPlayerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_compile_definitions(VideoPlayerTest PRIVATE WITH_TESTS)
target_include_directories(VideoPlayerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(VideoPlayerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(VideoPlayerTest PRIVATE BLUEBUILD)

if(APPLE)
    target_link_libraries(VideoPlayerTest PRIVATE "-framework CoreFoundation")
elseif(WIN32)
    target_link_libraries(VideoPlayerTest PRIVATE Winmm)
endif()

set_target_properties(VideoPlayerTest PROPERTIES FOLDER "Tests")
