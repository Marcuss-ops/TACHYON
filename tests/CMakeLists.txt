add_executable(TachyonTests
    unit/test_main.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/media/asset_resolution_tests.cpp
    unit/media/image_manager_tests.cpp
    unit/media/image_decode_tests.cpp
     unit/renderer2d/composition/framebuffer_tests.cpp
     unit/renderer2d/pipeline/scene3d_bridge_tests.cpp
     unit/renderer2d/rasterization/rasterizer_tests.cpp
    unit/renderer2d/utilities/surface_tests.cpp
    unit/renderer2d/composition/draw_list_builder_tests.cpp
    unit/renderer2d/effects/blend_modes_tests.cpp
    # unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
    unit/renderer2d/rasterization/path_rasterizer_tests.cpp
    unit/renderer2d/rasterization/path_rasterizer_aa_tests.cpp
    unit/renderer2d/utilities/tiling_tests.cpp
    unit/renderer2d/effects/effect_host_tests.cpp
    unit/studio/studio_library_tests.cpp
    unit/runtime/core/frame_cache_tests.cpp
    unit/runtime/core/frame_cache_budget_tests.cpp
    unit/runtime/core/backbone_tests.cpp
    # unit/runtime/core/render_contract_tests.cpp
    unit/runtime/core/expression_vm_tests.cpp
    unit/runtime/core/determinism_tests.cpp
    unit/runtime/io/frame_output_sink_tests.cpp
    unit/runtime/core/quality_tier_tests.cpp
    unit/runtime/execution/tiling_integration_tests.cpp
    unit/runtime/execution/frame_executor_tests.cpp
    unit/runtime/execution/tile_scheduler_tests.cpp
    unit/runtime/visual/visual_golden_tests.cpp
    unit/runtime/visual/visual_golden_verifier.cpp
    unit/runtime/visual/golden_fixtures.cpp
    unit/core/scene/evaluator_tests.cpp
    unit/core/scene/precomp_mask_tests.cpp
    unit/timeline/time_tests.cpp
    unit/timeline/camera_cuts_tests.cpp
    unit/core/spec/scene_spec_tests.cpp
    unit/core/camera/camera_shake_tests.cpp
    unit/core/properties/bezier_interpolator_tests.cpp
    unit/renderer2d/composition/matte_resolver_tests.cpp
    unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
    unit/editor/undo_manager_tests.cpp
    unit/editor/autosave_manager_tests.cpp
    unit/renderer3d/motion_blur_tests.cpp  # Re-enabled - was disabled for pre-existing build errors
    unit/core/rigging_maturation_tests.cpp
    unit/core/spec/parsing_tests.cpp
    unit/core/spec/validation_tests.cpp
    unit/core/spec/cli_tests.cpp
    unit/core/spec/scene_contract_tests.cpp
    unit/core/spec/render_job_tests.cpp
    unit/text/glyph_cache_tests.cpp
    unit/text/text_tests.cpp  # Re-enabled - was disabled for pre-existing build errors
    unit/text/editor_tests.cpp
    unit/audio/test_audio_pitch_correct.cpp  # Re-enabled - was disabled for pre-existing build errors
    unit/runtime/core/render_contract_tests.cpp
    integration/render_session_tests.cpp
    integration/render_batch_tests.cpp
    integration/parallax_cards_tests.cpp
    unit/renderer3d/temporal/time_remap_tests.cpp
    unit/renderer3d/temporal/frame_blend_tests.cpp
    # unit/renderer3d/effects/motion_blur_tests.cpp  # Duplicate - use unit/renderer3d/motion_blur_tests.cpp
    unit/renderer3d/temporal/rolling_shutter_tests.cpp
    unit/presets/preset_tests.cpp
    unit/presets/sfx_contract_tests.cpp
    unit/presets/shape_contract_tests.cpp
    unit/runtime/native_render_tests.cpp
)

set(TachyonTrackerTestSources
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
)

if(TACHYON_ENABLE_TRACKER)
    target_sources(TachyonTests PRIVATE ${TachyonTrackerTestSources})
endif()

set(TACHYON_TESTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
string(REPLACE "\\" "/" TACHYON_TESTS_SOURCE_DIR "${TACHYON_TESTS_SOURCE_DIR}")

target_compile_definitions(TachyonTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonCore
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonTests PRIVATE /FS)
endif()

add_executable(Phase1Tests unit/text/phase1_tests.cpp)
target_link_libraries(Phase1Tests PRIVATE TachyonCore)
target_compile_definitions(Phase1Tests PRIVATE TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}")

add_test(
    NAME TachyonTests
    COMMAND TachyonTests
)

add_test(
    NAME TachyonSmokeTests
    COMMAND TachyonTests
)

set_tests_properties(TachyonSmokeTests PROPERTIES
    ENVIRONMENT "TACHYON_TEST_FILTER=math,scene_spec,render_job,asset_resolution"
)
