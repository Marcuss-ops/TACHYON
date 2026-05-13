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
include(cmake/CoreTests.cmake)
include(cmake/ArchitectureTests.cmake)
include(cmake/SceneTests.cmake)
include(cmake/ToolTests.cmake)
include(cmake/RuntimeTests.cmake)
include(cmake/ContentTests.cmake)
include(cmake/RenderTests.cmake)
include(cmake/SmokeTests.cmake)

# Labs
if(TACHYON_ENABLE_OUTPUT)
    add_executable(transition_perf_lab perf/transition_perf_lab.cpp)
    target_link_libraries(transition_perf_lab PRIVATE 
        TachyonCore 
        TachyonOutput 
        TachyonRenderer2D
        TachyonPresets
        TachyonRuntimeTelemetry
        TachyonPlatform
        TachyonTimeline
    )
    if(TACHYON_ENABLE_MEDIA)
        target_link_libraries(transition_perf_lab PRIVATE TachyonMedia)
    endif()
    tachyon_configure_common(transition_perf_lab)
endif()

if(TACHYON_ENABLE_OUTPUT)
    add_executable(tachyon_bench perf/tachyon_bench.cpp)
    target_link_libraries(tachyon_bench PRIVATE 
        TachyonCore 
        TachyonOutput 
        TachyonRenderer2D
        TachyonPresets
        TachyonRuntimeTelemetry
        TachyonPlatform
        TachyonTimeline
    )
    if(TACHYON_ENABLE_MEDIA)
        target_link_libraries(tachyon_bench PRIVATE TachyonMedia)
    endif()
    tachyon_configure_common(tachyon_bench)
endif()

