cmake_minimum_required(VERSION 3.20)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)

set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_FILEFORMAT_TGA 1 CACHE STRING "" FORCE)
set(SUPPORT_CUSTOM_FRAME_CONTROL OFF CACHE BOOL "" FORCE)
set(SUPPORT_BUSY_WAIT_LOOP OFF CACHE BOOL "" FORCE)
set(SUPPORT_PARTIALBUSY_WAIT_LOOP ON CACHE BOOL "" FORCE)
set(SUPPORT_WINMM_HIGHRES_TIMER ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(SOUND_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/sound")

file(GLOB_RECURSE WAV_ASSETS CONFIGURE_DEPENDS "${ASSETS_DIR}/*.wav")

add_custom_command(
    OUTPUT "${SOUND_OUT_DIR}/samples.h"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sos_encode_sample.py"
            --wav-dir "${ASSETS_DIR}"
            --out-dir "${SOUND_OUT_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sos_encode_sample.py" ${WAV_ASSETS}
    COMMENT "Encoding audio..."
    VERBATIM
)

add_executable(xbox_startup
    "${SOUND_OUT_DIR}/samples.h"

    src/blob/blob.cpp
    src/blob/blob_bump.cpp
    src/blob/bloblet.cpp
    src/blob/blob_texgen.cpp
    src/blob/intensity_driver.cpp
    
    src/sos/sos_audio.cpp
    src/sos/sos_envelope.cpp
    src/sos/sos_sequencer.cpp
     
    src/util/noclip.cpp

    src/app.cpp
    src/main.cpp
)

target_link_libraries(xbox_startup PRIVATE raylib)

target_include_directories(xbox_startup PRIVATE "${SOUND_OUT_DIR}")

if (APPLE)
    target_link_libraries(xbox_startup PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()