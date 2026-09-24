include(CheckSourceCompiles)

set(EMBED_TO_INL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_fallback.py")

if(XBOX_BLOB_FORCE_NO_EMBED)
    set(HAVE_CXX_EMBED FALSE)
else()
    set(_embed_check_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/embed_check.bin")
    file(WRITE "${_embed_check_file}" "1")
    file(TO_CMAKE_PATH "${_embed_check_file}" _embed_check_file)

    check_source_compiles(CXX "
#ifdef __clang__
#pragma clang diagnostic ignored \"-Wc23-extensions\"
#endif
int main() {
    static constexpr unsigned char data[] = {
        #embed \"${_embed_check_file}\"
    };
    return data[0] == '1' ? 0 : 1;
}
" HAVE_CXX_EMBED)
endif()

function(xbb_target_embed target)
    if(HAVE_CXX_EMBED)
        target_compile_definitions(${target} PRIVATE HAS_EMBED)
        return()
    endif()

    set(shaders_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    if(NOT EXISTS "${shaders_src_dir}")
        return()
    endif()

    set(shaders_out_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(GLOB shader_sources CONFIGURE_DEPENDS "${shaders_src_dir}/*.vert" "${shaders_src_dir}/*.frag")

    set(generated_inls "")
    foreach(shader ${shader_sources})
        get_filename_component(shader_name "${shader}" NAME)
        set(out_file "${shaders_out_dir}/${shader_name}.inl")
        add_custom_command(
            OUTPUT "${out_file}"
            COMMAND Python3::Interpreter "${EMBED_TO_INL_SCRIPT}" "${shader}" "${out_file}"
            DEPENDS "${EMBED_TO_INL_SCRIPT}" "${shader}"
            COMMENT "Generating fallback ${shader_name}.inl"
            VERBATIM
        )
        list(APPEND generated_inls "${out_file}")
    endforeach()

    if(NOT generated_inls)
        return()
    endif()

    set(inl_target "${target}_embed_inl")
    add_custom_target(${inl_target} DEPENDS ${generated_inls})
    add_dependencies(${target} ${inl_target})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()