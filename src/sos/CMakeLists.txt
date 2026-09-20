add_library(sos_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_envelope.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_program.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_sequencer.cpp
)

target_link_libraries(sos_core PRIVATE raylib)

add_library(sos_boot STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/boot/sos_boot.cpp
    "${SOUND_OUT_DIR}/samples.h"
)

add_dependencies(sos_boot generate_sound_samples)
target_include_directories(sos_boot PRIVATE "${SOUND_OUT_DIR}")
target_link_libraries(sos_boot PUBLIC sos_core PRIVATE raylib)

add_library(sos
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_audio.cpp
)

target_link_libraries(sos PUBLIC sos_core PRIVATE raylib)
