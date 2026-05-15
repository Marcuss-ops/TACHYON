# ---------------------------------------------------------
# CORE DOMAIN: TachyonCoreTests
# Asset resolution, math, properties
# ---------------------------------------------------------
add_executable(TachyonCoreTests
    unit/mains/test_main_core.cpp
    unit/core/assets/asset_resolution_tests.cpp
    unit/mains/test_stubs.cpp
)

target_compile_definitions(TachyonCoreTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonCoreTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
)

add_test(
    NAME TachyonCoreTests
    COMMAND TachyonCoreTests
)
tachyon_set_test_labels(TachyonCoreTests core)
