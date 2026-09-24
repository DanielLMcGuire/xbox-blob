set(ANIM_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/animation")
set(ANIM_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/animation")

set(ANIM_PRIM_TYPES "${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_prim_types.h")

file(GLOB_RECURSE ANIM_OBJ_ASSETS CONFIGURE_DEPENDS "${ANIM_ASSETS_DIR}/*.obj")

add_custom_command(
    OUTPUT "${ANIM_OUT_DIR}/anim_data.h"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/anim_encode_obj.py"
            --assets-dir "${ANIM_ASSETS_DIR}"
            --prim-types "${ANIM_PRIM_TYPES}"
            --out-dir "${ANIM_OUT_DIR}"
            ${XBB_ASSET_ENCODE_ARGS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/anim_encode_obj.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/anim_common.py"
            "${XBB_EMBED_COMMON_SCRIPT}"
            "${XBB_ASSET_MODE_STAMP}"
            "${ANIM_PRIM_TYPES}"
            ${ANIM_OBJ_ASSETS}
    COMMENT "Encoding animation..."
    VERBATIM
)

add_custom_target(animation_generation DEPENDS "${ANIM_OUT_DIR}/anim_data.h")
