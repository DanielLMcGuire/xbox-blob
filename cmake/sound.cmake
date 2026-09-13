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