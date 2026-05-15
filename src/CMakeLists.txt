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
add_subdirectory(output)
add_subdirectory(text)
add_subdirectory(runtime)

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
)
tachyon_link_mimalloc(tachyon)

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
endif()
