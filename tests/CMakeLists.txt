# ---------------------------------------------------------
# Tachyon Test Suite - Main Entry Point
# ---------------------------------------------------------

set(TACHYON_TESTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
string(REPLACE "\\" "/" TACHYON_TESTS_SOURCE_DIR "${TACHYON_TESTS_SOURCE_DIR}")

# Utility functions for test properties
function(tachyon_set_test_labels test_name)
    if(ARGN)
        set_tests_properties(${test_name} PROPERTIES LABELS "${ARGN}")
    endif()
endfunction()

add_library(TachyonTestUtils STATIC
    unit/test_utils.cpp
    unit/image_comparison.cpp
)
target_compile_definitions(TachyonTestUtils PRIVATE
    TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)
target_include_directories(TachyonTestUtils PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
    ${CMAKE_SOURCE_DIR}/third_party/stb/include
)
target_link_libraries(TachyonTestUtils PUBLIC TachyonCore)

# Loadable Scene Modules
add_library(remotion_demo MODULE render/remotion_demo.cpp)
target_link_libraries(remotion_demo PRIVATE TachyonScene TachyonCore TachyonPresets TachyonCatalog)
set_target_properties(remotion_demo PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/render")

# Domain-specific test configurations
include(cmake/CoreTests.cmake)
include(cmake/RenderTests.cmake)
include(cmake/CliTests.cmake)
include(cmake/SceneTests.cmake)
include(cmake/RuntimeTests.cmake)
include(cmake/OperationsTests.cmake)
include(cmake/BackendTests.cmake)
include(cmake/OutputTests.cmake)
include(cmake/GoldenTests.cmake)
include(cmake/DeterminismTests.cmake)

# --- Header Smoke Tests ---
# This target ensures that key headers compile in isolation.
# It depends on small executables that just include one header.

add_custom_target(HeaderSmokeTests)

function(tachyon_add_layered_smoke_test layer_name source libs)
    set(target_name "HeaderSmoke${layer_name}")
    if(NOT TARGET ${target_name})
        add_executable(${target_name} ${source})
        tachyon_configure_common(${target_name})
        target_link_libraries(${target_name} PRIVATE ${libs})
        add_dependencies(HeaderSmokeTests ${target_name})
    else()
        target_sources(${target_name} PRIVATE ${source})
    endif()
endfunction()

# 1. Core Smoke (Strictly Core only)
tachyon_add_layered_smoke_test(Core unit/smoke/smoke_core.cpp "TachyonCore")

# 2. Backends Smoke (Backends + Core)
tachyon_add_layered_smoke_test(Backends unit/smoke/smoke_backends.cpp "TachyonBackends;TachyonCore")

# 3. Runtime Smoke (Runtime + Core)
tachyon_add_layered_smoke_test(Runtime unit/smoke/smoke_runtime.cpp "TachyonRuntime;TachyonCore")

# 4. CLI Smoke (Everything)
tachyon_add_layered_smoke_test(CLI unit/smoke/smoke_cli.cpp "TachyonCLI;TachyonRuntime;TachyonOperations")
