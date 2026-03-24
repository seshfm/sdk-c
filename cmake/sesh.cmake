# Sesh WASM toolchain file for CMake
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=path/to/sesh/cmake/sesh.cmake ..
#
# Automatically downloads wasi-sdk if WASI_SDK_PATH is not set.

set(SESH_WASI_SDK_VERSION "32")
set(SESH_WASI_SDK_TAG "wasi-sdk-${SESH_WASI_SDK_VERSION}")

# Detect host platform for wasi-sdk download
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(_WASI_ARCH "aarch64-linux")
    else()
        set(_WASI_ARCH "x86_64-linux")
    endif()
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        set(_WASI_ARCH "arm64-macos")
    else()
        set(_WASI_ARCH "x86_64-macos")
    endif()
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_WASI_ARCH "x86_64-windows")
endif()

# Auto-download wasi-sdk if not provided
if(NOT DEFINED WASI_SDK_PATH)
    if(DEFINED ENV{WASI_SDK_PATH})
        set(WASI_SDK_PATH "$ENV{WASI_SDK_PATH}")
    else()
        set(_WASI_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../.wasi-sdk")
        set(_WASI_SDK_STAMP "${_WASI_SDK_DIR}/.stamp-${SESH_WASI_SDK_TAG}")
        if(NOT EXISTS "${_WASI_SDK_STAMP}")
            set(_WASI_TAR "wasi-sdk-${SESH_WASI_SDK_VERSION}.0-${_WASI_ARCH}.tar.gz")
            set(_WASI_URL "https://github.com/aspect-build/aspect-cli/releases/download/${SESH_WASI_SDK_TAG}/${_WASI_TAR}")
            message(STATUS "Downloading wasi-sdk ${SESH_WASI_SDK_VERSION} for ${_WASI_ARCH}...")
            file(DOWNLOAD
                "https://github.com/WebAssembly/wasi-sdk/releases/download/${SESH_WASI_SDK_TAG}/${_WASI_TAR}"
                "${_WASI_SDK_DIR}/${_WASI_TAR}"
                SHOW_PROGRESS
                STATUS _dl_status)
            list(GET _dl_status 0 _dl_code)
            if(NOT _dl_code EQUAL 0)
                message(FATAL_ERROR "Failed to download wasi-sdk: ${_dl_status}")
            endif()
            file(ARCHIVE_EXTRACT
                INPUT "${_WASI_SDK_DIR}/${_WASI_TAR}"
                DESTINATION "${_WASI_SDK_DIR}")
            file(REMOVE "${_WASI_SDK_DIR}/${_WASI_TAR}")
            file(TOUCH "${_WASI_SDK_STAMP}")
        endif()
        file(GLOB _WASI_EXTRACTED "${_WASI_SDK_DIR}/wasi-sdk-*")
        list(GET _WASI_EXTRACTED 0 WASI_SDK_PATH)
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
set(CMAKE_CXX_FLAGS_INIT "-pthread -fno-exceptions -Wno-c23-extensions")

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
