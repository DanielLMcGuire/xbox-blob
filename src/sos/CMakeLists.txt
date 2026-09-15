add_library(sos
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_envelope.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sos_sequencer.cpp
)

target_link_libraries(sos PRIVATE raylib)