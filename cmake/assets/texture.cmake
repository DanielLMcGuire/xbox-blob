set(TEXTURE_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures")
set(TEXTURE_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/textures")

add_custom_command(
    OUTPUT "${TEXTURE_OUT_DIR}/tm_pixels.h"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tm_encode_tga.py"
            --tga "${TEXTURE_ASSETS_DIR}/tm_pixels.tga"
            --out-dir "${TEXTURE_OUT_DIR}"
            ${XBB_ASSET_ENCODE_ARGS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/tm_encode_tga.py"
            "${XBB_EMBED_COMMON_SCRIPT}"
            "${XBB_ASSET_MODE_STAMP}"
            "${TEXTURE_ASSETS_DIR}/tm_pixels.tga"
    COMMENT "Encoding textures..."
    VERBATIM
)

add_custom_target(texture_generation DEPENDS "${TEXTURE_OUT_DIR}/tm_pixels.h")
