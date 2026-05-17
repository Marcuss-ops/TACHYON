# Tachyon Root Source Module

# --- Module Configuration ---
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonMacros.cmake)

# --- Subdirectories (In dependency order) ---
# --- Subdirectories (In dependency order)

# Core and Base
add_subdirectory(core)
add_subdirectory(color)
add_subdirectory(timeline)
add_subdirectory(diagnostics)
add_subdirectory(catalog)

# Domain logic
add_subdirectory(presets)
add_subdirectory(core/scene)
add_subdirectory(renderer2d)

if(TACHYON_ENABLE_OUTPUT)
    add_subdirectory(output)
endif()

if(TACHYON_ENABLE_TEXT)
    add_subdirectory(text)
endif()

add_subdirectory(runtime)
add_subdirectory(backends)

# External/Operations
add_subdirectory(${CMAKE_SOURCE_DIR}/operations ${CMAKE_BINARY_DIR}/operations)

# CLI and Entry Points
add_subdirectory(cli)

# --- Main Executables ---

# Minimal render-focused executable
add_executable(tachyon main.cpp)
tachyon_configure_common(tachyon)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonCLI
        TachyonCatalog
        TachyonPresets
        TachyonDiagnostics
        TachyonOperations
        TachyonBackends
        TachyonRuntime
        TachyonOutput
)
tachyon_link_mimalloc(tachyon)
set_target_properties(tachyon PROPERTIES ENABLE_EXPORTS TRUE)

# --- Shared library (libtachyon.so / tachyon.dll) for Python/FFI ---
if(TACHYON_BUILD_SHARED_LIB)
    add_library(tachyon_lib SHARED core/c_api.cpp)
    set_target_properties(tachyon_lib PROPERTIES OUTPUT_NAME "tachyon")
    target_compile_definitions(tachyon_lib PRIVATE TACHYON_EXPORTS)
    tachyon_configure_common(tachyon_lib)
    target_link_libraries(tachyon_lib
        PRIVATE
            TachyonCLI
            TachyonCatalog
            TachyonPresets
            TachyonDiagnostics
            TachyonOperations
            TachyonBackends
            TachyonRuntime
            TachyonOutput
    )
    tachyon_link_mimalloc(tachyon_lib)
    message(STATUS "[Tachyon] Building shared library: libtachyon")
endif()

# Platform-specific post-build
if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET tachyon POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:tachyon>
            COMMAND_EXPAND_LISTS
        )
    endif()

    if(TACHYON_ENABLE_MEDIA AND TACHYON_FFMPEG_ROOT)
        file(GLOB TACHYON_FFMPEG_DLLS CONFIGURE_DEPENDS
            "${TACHYON_FFMPEG_ROOT}/bin/avcodec*.dll"
            "${TACHYON_FFMPEG_ROOT}/bin/avformat*.dll"
            "${TACHYON_FFMPEG_ROOT}/bin/avutil*.dll"
            "${TACHYON_FFMPEG_ROOT}/bin/swscale*.dll"
            "${TACHYON_FFMPEG_ROOT}/bin/swresample*.dll"
        )
        if(TACHYON_FFMPEG_DLLS)
            add_custom_command(TARGET tachyon POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${TACHYON_FFMPEG_DLLS}
                        $<TARGET_FILE_DIR:tachyon>
                COMMAND_EXPAND_LISTS
            )
        endif()
    endif()
endif()
