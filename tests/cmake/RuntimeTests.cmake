# ---------------------------------------------------------
# RUNTIME DOMAIN: TachyonRuntimeTests
# Engine state, event loops, session management
# ---------------------------------------------------------
add_executable(TachyonRuntimeTests
    unit/mains/test_main_runtime.cpp
    unit/mains/test_stubs.cpp
    unit/runtime/telemetry/sqlite_telemetry_store_tests.cpp
)


target_compile_definitions(TachyonRuntimeTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRuntimeTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
        TachyonRuntime
)

add_test(
    NAME TachyonRuntimeTests
    COMMAND TachyonRuntimeTests
)
tachyon_set_test_labels(TachyonRuntimeTests runtime)
