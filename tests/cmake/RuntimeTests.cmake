# ---------------------------------------------------------
# RUNTIME DOMAIN: TachyonRuntimeTests
# Engine state, event loops, session management
# ---------------------------------------------------------
add_executable(TachyonRuntimeTests
    unit/mains/test_main_runtime.cpp
    unit/mains/test_stubs.cpp
    unit/runtime/telemetry/sqlite_telemetry_store_tests.cpp
    unit/runtime/node_cache_tests.cpp
    unit/runtime/plugin_tests.cpp
    unit/runtime/sharded_lru_cache_tests.cpp
    unit/runtime/frame_cache_tests.cpp
    unit/runtime/thread_local_telemetry_tests.cpp
    unit/runtime/layer_kind_resolver_tests.cpp
    unit/runtime/render_determinism_tests.cpp
    unit/runtime/render_streaming_tests.cpp
    unit/runtime/layer_bounds_tests.cpp
    unit/runtime/render_job_tests.cpp
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
        TachyonScene
        TachyonRenderer2D
        TachyonRuntime
)

add_test(
    NAME TachyonRuntimeTests
    COMMAND TachyonRuntimeTests
)
tachyon_set_test_labels(TachyonRuntimeTests runtime)
