# ---------------------------------------------------------
# GOLDEN DOMAIN: TachyonGoldenTests
# Pixel-perfect regression tests
# ---------------------------------------------------------
add_executable(TachyonGoldenTests
    unit/mains/test_main_render.cpp
    unit/mains/test_stubs.cpp
    unit/render/transition_canary_tests.cpp
    unit/render/transition_gallery.cpp
    unit/render/remotion_demo_test.cpp
    unit/render/light_leak_lookbook_test.cpp
    unit/render/golden_smoke_tests.cpp
    golden/scenes/precomp_basic.cpp
    golden/scenes/text_basic.cpp
    golden/scenes/shape_basic.cpp
    golden/scenes/mask_basic.cpp
    golden/scenes/motion_blur_basic.cpp
)

target_compile_definitions(TachyonGoldenTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonGoldenTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonPresets
        TachyonCore
        TachyonRenderer2D
        TachyonRuntime
)

add_test(
    NAME TachyonGoldenTests
    COMMAND TachyonGoldenTests
)
tachyon_set_test_labels(TachyonGoldenTests golden)
