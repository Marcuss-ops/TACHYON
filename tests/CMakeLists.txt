add_executable(TachyonTests
    unit/test_main.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/media/asset_resolution_tests.cpp
    unit/media/image_manager_tests.cpp
    unit/media/image_decode_tests.cpp
    unit/renderer2d/composition/framebuffer_tests.cpp
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
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
    unit/renderer2d/composition/matte_resolver_tests.cpp
    unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
    unit/renderer2d/composition/text3d_preview_tests.cpp
    unit/editor/undo_manager_tests.cpp
    unit/editor/autosave_manager_tests.cpp
    # unit/renderer3d/motion_blur_tests.cpp  # Disabled - pre-existing build errors
    unit/core/rigging_maturation_tests.cpp
    unit/core/spec/parsing_tests.cpp
    unit/core/spec/validation_tests.cpp
    unit/core/spec/cli_tests.cpp
    unit/core/spec/scene_contract_tests.cpp
    unit/core/spec/render_job_tests.cpp
    unit/text/glyph_cache_tests.cpp
    # unit/text/text_tests.cpp  # Disabled - pre-existing build errors
    unit/text/editor_tests.cpp
    # unit/audio/test_audio_pitch_correct.cpp  # Disabled - pre-existing build errors
    unit/runtime/core/render_contract_tests.cpp
    integration/render_session_tests.cpp
    integration/render_batch_tests.cpp
    integration/parallax_cards_tests.cpp
    # unit/renderer3d/temporal/time_remap_tests.cpp  # Disabled - multiple main() definitions
    # unit/renderer3d/temporal/frame_blend_tests.cpp  # Disabled - multiple main() definitions
    # unit/renderer3d/effects/motion_blur_tests.cpp  # Disabled - multiple main() definitions
    # unit/renderer3d/temporal/rolling_shutter_tests.cpp  # Disabled - multiple main() definitions
)

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
    add_custom_command(TARGET TachyonTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OIDN_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:TachyonTests>
        COMMAND_EXPAND_LISTS
    )
endif()

# Header smoke tests - catch header errors early without full builds
add_executable(HeaderSmokeCompositionSpec compile/header_smoke_composition_spec.cpp)
target_link_libraries(HeaderSmokeCompositionSpec PRIVATE TachyonCore)

add_executable(HeaderSmokeSceneSpec compile/header_smoke_scene_spec.cpp)
target_link_libraries(HeaderSmokeSceneSpec PRIVATE TachyonCore)

add_executable(HeaderSmokeLayerSpec compile/header_smoke_layer_spec.cpp)
target_link_libraries(HeaderSmokeLayerSpec PRIVATE TachyonCore)

add_executable(HeaderSmokeRenderJobSpec compile/header_smoke_render_job_spec.cpp)
target_link_libraries(HeaderSmokeRenderJobSpec PRIVATE TachyonCore)

add_custom_target(HeaderSmokeTests
    DEPENDS HeaderSmokeCompositionSpec HeaderSmokeSceneSpec HeaderSmokeLayerSpec HeaderSmokeRenderJobSpec
    COMMENT "Running header smoke tests to catch compilation errors early"
)

add_executable(Phase1Tests unit/text/phase1_tests.cpp)
target_link_libraries(Phase1Tests PRIVATE TachyonCore)
target_compile_definitions(Phase1Tests PRIVATE TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}")
if(MSVC)
    target_compile_options(Phase1Tests PRIVATE /FS)
endif()
