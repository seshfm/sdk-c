# Sesh WASM toolchain file for CMake
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/sesh.cmake -DWASI_SDK_PATH=/path/to/wasi-sdk ..

if(NOT DEFINED WASI_SDK_PATH)
    if(DEFINED ENV{WASI_SDK_PATH})
        set(WASI_SDK_PATH "$ENV{WASI_SDK_PATH}")
    else()
        message(FATAL_ERROR "Set WASI_SDK_PATH to your wasi-sdk installation")
    endif()
endif()
set(WASI_SDK_PATH "${WASI_SDK_PATH}" CACHE PATH "Path to wasi-sdk" FORCE)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES WASI_SDK_PATH)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "${WASI_SDK_PATH}/bin/clang")
set(CMAKE_CXX_COMPILER "${WASI_SDK_PATH}/bin/clang++")

set(CMAKE_C_COMPILER_TARGET wasm32-wasip1-threads)
set(CMAKE_CXX_COMPILER_TARGET wasm32-wasip1-threads)

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT "-pthread -fno-exceptions")
set(CMAKE_CXX_FLAGS_INIT "-pthread -fno-exceptions")

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-nostartfiles -Wl,--no-entry -Wl,--import-memory -Wl,--shared-memory -Wl,--max-memory=4294967296")

# Sesh plugin exports
set(SESH_EXPORTS
    sesh_sdk_version sesh_alloc sesh_free
    sesh_create sesh_destroy sesh_params
    sesh_process sesh_draw sesh_draw_width
    __indirect_function_table __stack_pointer
    __tls_base __tls_size __tls_align
    __wasm_init_tls __data_end __heap_base)

function(sesh_plugin target)
    set_target_properties(${target} PROPERTIES SUFFIX ".wasm")
    foreach(sym IN LISTS SESH_EXPORTS)
        target_link_options(${target} PRIVATE "LINKER:--export=${sym}")
    endforeach()
endfunction()
