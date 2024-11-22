cmake_minimum_required(VERSION 3.16)
<<<<<<< HEAD
<<<<<<< HEAD
project(VideoPlayer VERSION 1.0.0)
=======
project(project_name VERSION 1.0.0)
>>>>>>> template/carbon-videoplayer-updates
=======
project(project_name VERSION 1.0.0)
>>>>>>> template/carbon-videoplayer-updates

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
<<<<<<< HEAD
<<<<<<< HEAD
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
    target_include_directories(VideoPlayer PRIVATE "$<TARGET_PROPERTY:trinity_stub,INTERFACE_INCLUDE_DIRECTORIES>")
elseif(APPLE)
    find_package(Trinity_metal REQUIRED NO_CMAKE_PATH)
    target_include_directories(VideoPlayer PRIVATE "$<TARGET_PROPERTY:trinity_metal,INTERFACE_INCLUDE_DIRECTORIES>")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()


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

if(WIN32)
    target_include_directories(VideoPlayerTest PRIVATE "$<TARGET_PROPERTY:trinity_stub,INTERFACE_INCLUDE_DIRECTORIES>")
elseif(APPLE)
    target_include_directories(VideoPlayerTest PRIVATE "$<TARGET_PROPERTY:trinity_metal,INTERFACE_INCLUDE_DIRECTORIES>")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

find_package(GTest REQUIRED)

target_include_directories(VideoPlayerTest PUBLIC "$<TARGET_PROPERTY:Blue,INTERFACE_INCLUDE_DIRECTORIES>")
target_include_directories(VideoPlayerTest PUBLIC "$<TARGET_PROPERTY:CarbonAudio,INTERFACE_INCLUDE_DIRECTORIES>")


target_link_libraries(VideoPlayerTest PRIVATE VORBIS VPX NestEgg GTest::GTest GTest::Main BlueExposure)

get_target_property(_SOURCES VideoPlayerTest SOURCES)
=======
=======
>>>>>>> template/carbon-videoplayer-updates

include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)

ccp_add_library(project_name SHARED)

get_target_property(_SOURCES project_name SOURCES)
<<<<<<< HEAD
>>>>>>> template/carbon-videoplayer-updates
=======
>>>>>>> template/carbon-videoplayer-updates
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

<<<<<<< HEAD
<<<<<<< HEAD
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
=======
=======
>>>>>>> template/carbon-videoplayer-updates
set_target_properties(project_name PROPERTIES OUTPUT_NAME "_project_name")

# Provide an install target if this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL project_name)
    # Determine if/when to generate/build documentation. The rules are as follows:
    # - If running locally - Build documentation is OFF by default
    # - If running on a build agent (TeamCity) - Build documentation is ON by default
    # - If -DBUILD_DOCUMENTATION=ON/OFF flag is explicitly set, use it.
    set(BUILD_DOCUMENTATION_DEFAULT_FLAG OFF)
    if(DEFINED ENV{TEAMCITY_VERSION})
        set(BUILD_DOCUMENTATION_DEFAULT_FLAG ON)
    endif()
    message(STATUS "Document generation settings: default=${BUILD_DOCUMENTATION_DEFAULT_FLAG}, option BUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}")
    option(BUILD_DOCUMENTATION "Build Documentation" ${BUILD_DOCUMENTATION_DEFAULT_FLAG})

    if(BUILD_DOCUMENTATION)
        # Find Doxygen, which is needed to parse C++ source files
        find_package(Doxygen)

        set(DOCUMENTATION_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)

        # Evaluate config file for Doxygen to input project values
        set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
        set(DOXYFILE_OUT ${DOCUMENTATION_OUT_DIR}/Doxyfile)
        configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

        set(DOXYGEN_OUT_DIR ${DOCUMENTATION_OUT_DIR}/xml)
        set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUT_DIR}/index.xml)

        # Regenerate with source changes
        add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                DEPENDS ${_SOURCES}
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                COMMENT "Running Doxygen"
                VERBATIM)

        add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

        # Set up sphinx build for Python autodoc comments.
        set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/doc/source)
        set(SPHINX_BUILD ${DOCUMENTATION_OUT_DIR}/build)

        # Select the correct pythonInterpreter to use based on build system.
        if(WIN32)
            set(CCP_PYTHON_INTERPRETER "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.bat")
        elseif(APPLE)
            set(CCP_PYTHON_INTERPRETER "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.sh")
        endif()

        # Run Sphinx against configuration file and Doxygen output
        add_custom_target(Sphinx ALL
                COMMAND ${CMAKE_COMMAND}
                -E env
                PYTHONPATH="$<TARGET_FILE_DIR:project_name>\;${CMAKE_SOURCE_DIR}/python"
                BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>
                ${CCP_PYTHON_INTERPRETER}
                -m sphinx build -E -b html
                -Dbreathe_projects.doxygen=${DOXYGEN_OUT_DIR}
                -c ${SPHINX_SOURCE}
                ${SPHINX_SOURCE} ${SPHINX_BUILD}
                WORKING_DIRECTORY ${BRANCH_ROOT_DIR}
                DEPENDS ${DOXYGEN_INDEX_FILE}
                COMMENT "Generating documentation with Sphinx")

        # Install rule for documentation
        install(DIRECTORY ${SPHINX_BUILD} DESTINATION documentation)
    endif()

    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
            TARGETS project_name
            EXPORT project_nameTarget
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    configure_ccp_vendor_config_file(
            TARGET project_name
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/project_nameConfig.cmake"
            DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/project_nameConfig.cmake DESTINATION .)

    # Install python package
    install(DIRECTORY python/ DESTINATION python
            FILES_MATCHING
            PATTERN "*.py"
            PATTERN "package.yaml"
    )
endif()
<<<<<<< HEAD
>>>>>>> template/carbon-videoplayer-updates
=======
>>>>>>> template/carbon-videoplayer-updates
