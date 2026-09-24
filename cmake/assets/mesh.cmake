set(MESH_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/meshes")
set(MESH_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/meshes")

file(GLOB_RECURSE OBJ_ASSETS CONFIGURE_DEPENDS "${MESH_ASSETS_DIR}/*.obj")

add_custom_command(
    OUTPUT "${MESH_OUT_DIR}/logo_data.h" "${MESH_OUT_DIR}/text_data.h"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mesh_encode_obj.py"
            --assets-dir "${MESH_ASSETS_DIR}"
            --out-dir "${MESH_OUT_DIR}"
            ${XBB_ASSET_ENCODE_ARGS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mesh_encode_obj.py"
            "${XBB_EMBED_COMMON_SCRIPT}"
            "${XBB_ASSET_MODE_STAMP}"
            ${OBJ_ASSETS}
    COMMENT "Encoding meshes..."
    VERBATIM
)

add_custom_target(mesh_generation DEPENDS "${MESH_OUT_DIR}/logo_data.h" "${MESH_OUT_DIR}/text_data.h")
