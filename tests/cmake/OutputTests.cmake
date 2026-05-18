# ---------------------------------------------------------
# OUTPUT DOMAIN: TachyonOutputTests
# FFmpeg commands, hardware detection, sink logic
# ---------------------------------------------------------
add_executable(TachyonOutputTests
    unit/mains/test_main_output.cpp
    unit/mains/test_stubs.cpp
    unit/output/ffmpeg_caps_tests.cpp
    unit/output/hardware_encoder_detector_tests.cpp
    unit/output/shared_memory_sink_tests.cpp
)

target_compile_definitions(TachyonOutputTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonOutputTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
        TachyonRenderer2D
        TachyonRuntime
        TachyonOutput
)

add_test(
    NAME TachyonOutputTests
    COMMAND TachyonOutputTests
)
tachyon_set_test_labels(TachyonOutputTests output)
