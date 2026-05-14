set(RUNTIME_TEST_SOURCES
    unit/runtime/core/determinism_tests.cpp
    unit/runtime/execution/frame_executor_tests.cpp
    unit/runtime/hash/scene_hash_coverage_tests.cpp
)

add_executable(TachyonRuntimeTests 
    unit/mains/test_main_runtime.cpp
    ${RUNTIME_TEST_SOURCES}
)

target_link_libraries(TachyonRuntimeTests 
    PRIVATE 
        TachyonRuntime
        TachyonRenderer2D
        TachyonCore
        TachyonScene
        TachyonMedia
        TachyonOutput
        TachyonTimeline
        TachyonPlatform
        TachyonTestUtils
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME TachyonRuntimeTests COMMAND TachyonRuntimeTests)
tachyon_set_test_labels(TachyonRuntimeTests runtime)
