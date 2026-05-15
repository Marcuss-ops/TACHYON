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

# Shared Test Utilities library
add_library(TachyonTestUtils STATIC
    unit/test_utils.cpp
)
target_include_directories(TachyonTestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unit)
target_link_libraries(TachyonTestUtils PUBLIC TachyonCore)

# Domain-specific test configurations
include(cmake/RenderTests.cmake)
include(cmake/CliTests.cmake)
include(cmake/SceneTests.cmake)
include(cmake/RuntimeTests.cmake)
include(cmake/OperationsTests.cmake)
include(cmake/GoldenTests.cmake)

# --- Header Smoke Tests ---
# This target ensures that key headers compile in isolation.
# It depends on small executables that just include one header.

add_custom_target(HeaderSmokeTests)

function(tachyon_add_smoke_test name source)
    set(target_name "HeaderSmoke${name}")
    add_executable(${target_name} ${source})
    tachyon_configure_common(${target_name})
    target_link_libraries(${target_name} PRIVATE TachyonCLI TachyonRuntime TachyonOperations)
    add_dependencies(HeaderSmokeTests ${target_name})
endfunction()

tachyon_add_smoke_test(Core unit/smoke/smoke_core.cpp)
tachyon_add_smoke_test(Scene unit/smoke/smoke_scene.cpp)
