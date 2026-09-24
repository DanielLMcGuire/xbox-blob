set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/samples")
set(SOUND_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/sound")

file(GLOB_RECURSE WAV_ASSETS CONFIGURE_DEPENDS "${ASSETS_DIR}/*.wav")

add_custom_command(
    OUTPUT "${SOUND_OUT_DIR}/samples.h"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sos_encode_sample.py"
            --wav-dir "${ASSETS_DIR}"
            --out-dir "${SOUND_OUT_DIR}"
            ${XBB_ASSET_ENCODE_ARGS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sos_encode_sample.py"
            "${XBB_EMBED_COMMON_SCRIPT}"
            "${XBB_ASSET_MODE_STAMP}"
            ${WAV_ASSETS}
    COMMENT "Encoding audio..."
    VERBATIM
)

add_custom_target(sample_generation DEPENDS "${SOUND_OUT_DIR}/samples.h")