set(TACHYON_TESTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
string(REPLACE "\\" "/" TACHYON_TESTS_SOURCE_DIR "${TACHYON_TESTS_SOURCE_DIR}")

add_executable(TachyonTests
    unit/test_main_core.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/core/camera/camera_shake_tests.cpp
    unit/core/properties/bezier_interpolator_tests.cpp
    unit/timeline/camera_cuts_tests.cpp
    unit/core/scene/builder_preset_tests.cpp
)

target_compile_definitions(TachyonTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonCore
        TachyonColor
        TachyonScene
        TachyonText
        TachyonRenderer2D
        TachyonPlatform
        TachyonRuntime
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonTests PRIVATE /FS)
endif()

add_executable(TachyonColorTests
    unit/color/color_tests.cpp
    unit/color/blend_kernel_tests.cpp
)

target_link_libraries(TachyonColorTests
    PRIVATE
        TachyonColor
)

if(MSVC)
    target_compile_options(TachyonColorTests PRIVATE /FS)
endif()

add_executable(TachyonSceneTests
    unit/test_main_scene.cpp
    unit/core/scene/evaluator_tests.cpp
    unit/timeline/time_tests.cpp
    unit/core/analysis/scene_inspector_tests.cpp
    unit/core/analysis/motion_map_tests.cpp
    unit/core/camera/default_camera_tests.cpp
)

target_compile_definitions(TachyonSceneTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonSceneTests
    PRIVATE
        TachyonCore
        TachyonScene
        TachyonMedia
        TachyonAudio
        TachyonRenderer2D
        TachyonPlatform
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonSceneTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonSceneTests PRIVATE /FS)
endif()

add_executable(TachyonCliTests
    unit/test_main_cli.cpp
    unit/core/spec/cli_tests.cpp
    unit/core/spec/scene_spec_tests.cpp
    integration/vertical_slice_tests.cpp
)

target_compile_definitions(TachyonCliTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonCliTests
    PRIVATE
        TachyonCLI
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonCliTests PRIVATE TachyonTracker)
endif()

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonCliTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonCliTests PRIVATE /FS)
endif()

add_executable(TachyonEditorTests
    unit/test_main_editor.cpp
    unit/editor/undo_manager_tests.cpp
    unit/editor/autosave_manager_tests.cpp
)

target_compile_definitions(TachyonEditorTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonEditorTests
    PRIVATE
        TachyonEditor
        TachyonCore
        TachyonScene
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonEditorTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonEditorTests PRIVATE /FS)
endif()

add_executable(TachyonRuntimeTests
    unit/test_main_runtime.cpp
    unit/core/spec/render_job_tests.cpp
    unit/runtime/core/frame_cache_tests.cpp
    unit/runtime/core/frame_cache_budget_tests.cpp
    unit/runtime/core/backbone_tests.cpp
    unit/runtime/core/expression_vm_tests.cpp
    unit/runtime/core/determinism_tests.cpp
    unit/runtime/io/frame_output_sink_tests.cpp
    unit/runtime/core/quality_tier_tests.cpp
    unit/runtime/execution/tiling_integration_tests.cpp
    unit/runtime/execution/frame_executor_tests.cpp
    unit/runtime/execution/frame_range_tests.cpp
    unit/runtime/execution/tile_scheduler_tests.cpp
    unit/runtime/frame_adapter_tests.cpp
)

target_compile_definitions(TachyonRuntimeTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRuntimeTests
    PRIVATE
        TachyonRuntimeEngine
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonRuntimeTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonRuntimeTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonRuntimeTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonRuntimeTests PRIVATE /FS)
endif()

add_executable(TachyonContentTests
    unit/test_main_content.cpp
    unit/studio/studio_library_tests.cpp
    unit/text/glyph_cache_tests.cpp
    unit/text/text_tests.cpp
    unit/text/text_preset_tests.cpp
    unit/text/text_preset_registry_tests.cpp
    unit/audio/test_audio_pitch_correct.cpp
    unit/audio/test_audio_trim.cpp
    unit/presets/background_preset_registry_tests.cpp
    unit/presets/transition_builder_tests.cpp
    unit/presets/sfx_contract_tests.cpp
    unit/presets/transition_preset_registry_tests.cpp
)

target_compile_definitions(TachyonContentTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonContentTests
    PRIVATE
        TachyonCore
        TachyonScene
        TachyonText
        TachyonAudio
        TachyonStudio
        TachyonRenderer2D
        TachyonPlatform
        TachyonRuntime
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonContentTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonContentTests PRIVATE /FS)
endif()

add_executable(TachyonRenderTests
    unit/test_main_render.cpp
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
    unit/renderer2d/composition/matte_resolver_tests.cpp
    unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
    unit/renderer3d/motion_blur_tests.cpp
    integration/render_session_tests.cpp
    integration/parallax_cards_tests.cpp
    integration/png_3d_validation_tests.cpp
    unit/renderer3d/temporal/time_remap_tests.cpp
    unit/renderer3d/temporal/frame_blend_tests.cpp
    # unit/renderer3d/effects/motion_blur_tests.cpp  # Duplicate - use unit/renderer3d/motion_blur_tests.cpp
    unit/renderer3d/temporal/rolling_shutter_tests.cpp
    unit/renderer3d/modifiers/modifier_tests.cpp
    unit/runtime/native_render_tests.cpp
    unit/runtime/profiling/render_profiler_tests.cpp
    integration/scene3d_smoke_tests.cpp
)

set(TachyonTrackerTestSources
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
)

if(TACHYON_ENABLE_TRACKER)
    target_sources(TachyonRenderTests PRIVATE ${TachyonTrackerTestSources})
endif()

target_compile_definitions(TachyonRenderTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRenderTests
    PRIVATE
        TachyonRuntimeEngine
        TachyonStudio
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonRenderTests PRIVATE TachyonTracker)
endif()

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonRenderTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonRenderTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonRenderTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonRenderTests PRIVATE /FS)
endif()

# Header smoke tests - catch header errors early without full builds
add_executable(HeaderSmokeCompositionSpec compile/header_smoke_composition_spec.cpp)
target_link_libraries(HeaderSmokeCompositionSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeSceneSpec compile/header_smoke_scene_spec.cpp)
target_link_libraries(HeaderSmokeSceneSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeLayerSpec compile/header_smoke_layer_spec.cpp)
target_link_libraries(HeaderSmokeLayerSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeSceneBuilder compile/header_smoke_scene_builder.cpp)
target_link_libraries(HeaderSmokeSceneBuilder PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeRenderJobSpec compile/header_smoke_render_job_spec.cpp)
target_link_libraries(HeaderSmokeRenderJobSpec PRIVATE TachyonCore TachyonScene TachyonRuntime)

add_custom_target(HeaderSmokeTests
    DEPENDS HeaderSmokeCompositionSpec HeaderSmokeSceneSpec HeaderSmokeLayerSpec HeaderSmokeSceneBuilder HeaderSmokeRenderJobSpec
    COMMENT "Running header smoke tests to catch compilation errors early"
)

add_executable(Phase1Tests unit/text/phase1_tests.cpp)
target_link_libraries(Phase1Tests PRIVATE TachyonCore TachyonScene TachyonText)
target_compile_definitions(Phase1Tests PRIVATE TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}")

add_test(
    NAME TachyonTests
    COMMAND TachyonTests
)

add_test(
    NAME TachyonColorTests
    COMMAND TachyonColorTests
)

add_test(
    NAME TachyonSceneTests
    COMMAND TachyonSceneTests
)

add_test(
    NAME TachyonCliTests
    COMMAND TachyonCliTests
)

add_test(
    NAME TachyonEditorTests
    COMMAND TachyonEditorTests
)

add_test(
    NAME TachyonRuntimeTests
    COMMAND TachyonRuntimeTests
)

add_test(
    NAME TachyonContentTests
    COMMAND TachyonContentTests
)

add_test(
    NAME TachyonRenderTests
    COMMAND TachyonRenderTests
)

add_test(
    NAME TachyonSmokeTests
    COMMAND TachyonTests
)

set_tests_properties(TachyonSmokeTests PROPERTIES
    ENVIRONMENT "TACHYON_TEST_FILTER=math,property,expression,camera_cuts"
)
