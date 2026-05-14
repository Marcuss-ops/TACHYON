# ---------------------------------------------------------
# 6. RENDER DOMAIN: TachyonRenderTests
# renderer2d/*, asset resolution
# ---------------------------------------------------------
add_executable(TachyonRenderTests
    unit/mains/test_canary_main.cpp
    unit/render/transition_canary_tests.cpp
    unit/render/transition_gallery.cpp
    unit/render/remotion_demo_test.cpp
    unit/render/light_leak_lookbook_test.cpp
)

target_compile_definitions(TachyonRenderTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRenderTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonPresets
        TachyonCore
        TachyonRenderer2D
        TachyonRuntime
        TachyonTimeline
)

add_test(
    NAME TachyonRenderTests
    COMMAND TachyonRenderTests
)
tachyon_set_test_labels(TachyonRenderTests render)
