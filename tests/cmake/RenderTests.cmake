set(TachyonTrackerTestSources
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
)

# ---------------------------------------------------------
# 6. RENDER DOMAIN: TachyonRenderTests
# renderer2d/*, renderer3d/*, asset resolution
# ---------------------------------------------------------
add_executable(TachyonRenderTests
    unit/test_main_render.cpp
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
)

if(TACHYON_ENABLE_3D)
    target_sources(TachyonRenderTests PRIVATE
        unit/renderer2d/pipeline/scene3d_bridge_tests.cpp
        unit/renderer3d/motion_blur_tests.cpp
        unit/renderer3d/modifiers/modifier_tests.cpp
    )
    target_link_libraries(TachyonRenderTests PRIVATE TachyonRenderer3D)
    
    add_executable(TachyonRenderPipelineTests
        unit/test_main_render_pipeline.cpp
        integration/render_session_tests.cpp
        integration/parallax_cards_tests.cpp
        integration/png_3d_validation_tests.cpp
        unit/renderer3d/motion_blur_tests.cpp
        unit/renderer3d/temporal/time_remap_tests.cpp
        unit/renderer3d/temporal/frame_blend_tests.cpp
        unit/renderer3d/temporal/rolling_shutter_tests.cpp
        unit/renderer3d/modifiers/modifier_tests.cpp
    )
    
    target_compile_definitions(TachyonRenderPipelineTests
        PRIVATE
            TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
    )

    target_link_libraries(TachyonRenderPipelineTests
        PRIVATE
            TachyonTestUtils
            TachyonRuntime
            TachyonRenderer2D
            TachyonRenderer3D
    )
endif()

if(TACHYON_ENABLE_TRACKER)
    target_sources(TachyonRenderTests PRIVATE ${TachyonTrackerTestSources})
    target_link_libraries(TachyonRenderTests PRIVATE TachyonTracker)
endif()

add_test(
    NAME TachyonRenderTests
    COMMAND TachyonRenderTests
)
tachyon_set_test_labels(TachyonRenderTests render)

if(TACHYON_ENABLE_3D)
    add_test(
        NAME TachyonRenderPipelineTests
        COMMAND TachyonRenderPipelineTests
    )
    tachyon_set_test_labels(TachyonRenderPipelineTests render deterministic)

    add_test(
        NAME TachyonScene3DSmokeTests
        COMMAND TachyonScene3DSmokeTests
    )
    tachyon_set_test_labels(TachyonScene3DSmokeTests render)
endif()
