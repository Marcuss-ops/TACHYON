set(TachyonTrackerTestSources
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
)

# ---------------------------------------------------------
# 6. RENDER DOMAIN: TachyonRenderTests
# renderer2d/*, asset resolution
# ---------------------------------------------------------
add_executable(TachyonRenderTests
    unit/mains/test_main_render.cpp
    unit/render/render_intent_tests.cpp
    unit/media/asset_resolution_tests.cpp
    unit/media/image_manager_tests.cpp
    unit/media/image_decode_tests.cpp
    unit/media/media_manager_cache_tests.cpp
    unit/renderer2d/composition/framebuffer_tests.cpp
    unit/renderer2d/rasterization/rasterizer_tests.cpp
    unit/renderer2d/utilities/surface_tests.cpp
    unit/renderer2d/composition/draw_list_builder_tests.cpp
    unit/renderer2d/effects/blend_modes_tests.cpp
    unit/renderer2d/effects/transition_fast_paths_tests.cpp
    unit/renderer2d/effects/light_leak_transitions_tests.cpp
    unit/renderer2d/rasterization/path_rasterizer_tests.cpp
    unit/renderer2d/rasterization/path_rasterizer_aa_tests.cpp
    unit/renderer2d/utilities/tiling_tests.cpp
    unit/renderer2d/effects/effect_host_tests.cpp
    unit/renderer2d/composition/matte_resolver_tests.cpp
    unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
    unit/runtime/native_render_tests.cpp
)

target_compile_definitions(TachyonRenderTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRenderTests
    PRIVATE
        TachyonTestUtils
        TachyonPresets
        TachyonRenderer2D
        TachyonRuntime
        TachyonPlatform
        TachyonTimeline
)

if(TACHYON_ENABLE_TRACKER)
    target_sources(TachyonRenderTests PRIVATE ${TachyonTrackerTestSources})
    target_link_libraries(TachyonRenderTests PRIVATE TachyonTracker)
endif()

add_test(
    NAME TachyonRenderTests
    COMMAND TachyonRenderTests
)
tachyon_set_test_labels(TachyonRenderTests render)
