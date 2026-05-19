# ---------------------------------------------------------
# DETERMINISM DOMAIN: TachyonDeterminismTests
# Strict zero-tolerance byte-level regressions
# ---------------------------------------------------------
add_executable(TachyonDeterminismTests
    unit/mains/test_main_determinism.cpp
    unit/mains/test_stubs.cpp
    unit/determinism/determinism_utils.cpp
    unit/render/determinism_frame_tests.cpp
    unit/render/determinism_sequence_tests.cpp
    unit/render/determinism_cache_tests.cpp
    unit/render/determinism_parallel_tests.cpp
)

target_compile_definitions(TachyonDeterminismTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonDeterminismTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonPresets
        TachyonCore
        TachyonScene
        TachyonRenderer2D
        TachyonRuntime
)

add_test(
    NAME TachyonDeterminismTests
    COMMAND TachyonDeterminismTests
)
tachyon_set_test_labels(TachyonDeterminismTests determinism)
