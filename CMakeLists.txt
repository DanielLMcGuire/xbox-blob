cmake_minimum_required(VERSION 3.20)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_executable(xbox_startup
    src/main.cpp
    src/blob/blob.cpp
    src/sos/sos_audio.cpp
    src/sos/sos_envelope.cpp
    src/sos/sos_sequencer.cpp
)

target_link_libraries(xbox_startup PRIVATE raylib)

if (APPLE)
    target_link_libraries(xbox_startup PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/shaders/*"
)

set(SHADER_STAMP "${CMAKE_CURRENT_BINARY_DIR}/shaders_copy.stamp")

add_custom_command(
    OUTPUT "${SHADER_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/shaders"
        "$<TARGET_FILE_DIR:xbox_startup>/shaders"
    COMMAND ${CMAKE_COMMAND} -E touch "${SHADER_STAMP}"
    DEPENDS ${SHADER_FILES}
    COMMENT "Copying shaders to output directory..."
)

add_custom_target(xbox_blob_shaders ALL DEPENDS "${SHADER_STAMP}")

add_dependencies(xbox_startup xbox_blob_shaders)