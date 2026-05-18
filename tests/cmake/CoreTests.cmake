# ---------------------------------------------------------
# CORE DOMAIN: TachyonCoreTests
# Asset resolution, math, properties
# ---------------------------------------------------------
tachyon_add_test_suite(
    TARGET TachyonCoreTests
    SOURCES
        unit/mains/test_main_core.cpp
        unit/core/assets/asset_resolution_tests.cpp
        unit/core/transition/transition_simd_kernels_tests.cpp
        unit/mains/test_stubs.cpp
        unit/diagnostics/test_trace_scope.cpp
        unit/diagnostics/test_logging.cpp
        unit/core/test_c_api.cpp
    LIBS
        -Wl,--start-group
        TachyonBackends
        -Wl,--end-group
        TachyonPlatform
        TachyonTestUtils
        TachyonDiagnostics
        -Wl,--whole-archive
        TachyonRenderer2D
        TachyonCore
        TachyonScene
        TachyonCLI
        TachyonBindingsC
        -Wl,--no-whole-archive
        TachyonRuntime
    LABELS core
)
