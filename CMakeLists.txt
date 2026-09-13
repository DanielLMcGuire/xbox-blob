cmake_minimum_required(VERSION 3.20)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("cmake/raylib.cmake")
include("cmake/sound.cmake")

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