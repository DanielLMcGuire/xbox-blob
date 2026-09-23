include("${CMAKE_CURRENT_LIST_DIR}/CompilerFlags.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/InlineAssembly.cmake")

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(${CMAKE_CURRENT_LIST_DIR}/ClangFix.cmake)
elseif(WIN32 AND MSVC)
    include(${CMAKE_CURRENT_LIST_DIR}/MSVCFix.cmake)
endif()