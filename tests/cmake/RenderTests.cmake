# ---------------------------------------------------------
# 6. RENDER DOMAIN: TachyonRenderTests
# renderer2d/*, asset resolution
# ---------------------------------------------------------
tachyon_add_test_suite(
    TARGET TachyonRenderTests
    SOURCES
        unit/mains/test_canary_main.cpp
        unit/render/transition_canary_tests.cpp
        unit/render/transition_gallery.cpp
        unit/render/remotion_demo_test.cpp
        unit/render/light_leak_lookbook_test.cpp
        unit/renderer2d/geometry_tests.cpp
    LIBS
        TachyonPlatform
        TachyonTestUtils
        TachyonPresets
        TachyonCore
        TachyonRenderer2D
        TachyonRuntime
        TachyonTimeline
    LABELS render
)
