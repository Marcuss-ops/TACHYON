# ---------------------------------------------------------
# CORE DOMAIN: TachyonCoreTests
# Asset resolution, math, properties
# ---------------------------------------------------------
add_executable(TachyonCoreTests
    unit/mains/test_main_core.cpp
    unit/core/assets/asset_resolution_tests.cpp
    unit/core/transition/transition_simd_kernels_tests.cpp
    unit/mains/test_stubs.cpp
    unit/diagnostics/test_trace_scope.cpp
    unit/diagnostics/test_logging.cpp
    unit/core/test_c_api.cpp
)

target_compile_definitions(TachyonCoreTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonCoreTests
    PRIVATE
        TachyonBindingsC
        TachyonBackends
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
        TachyonDiagnostics
)

add_test(
    NAME TachyonCoreTests
    COMMAND TachyonCoreTests
)
tachyon_set_test_labels(TachyonCoreTests core)
