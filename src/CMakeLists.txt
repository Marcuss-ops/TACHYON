find_package(OpenGL REQUIRED)

file(GLOB_RECURSE TACHYON_ALL_CPP CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

set(TachyonCoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/gallery_gen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/animation_primitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/c_api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/report.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/transition_registry.cpp
)

set(TachyonSceneSources ${TACHYON_ALL_CPP})
list(FILTER TachyonSceneSources INCLUDE REGEX "/(core/(scene|spec)|presets|timeline)/|/(background_generator|camera_cut_contract)\\.cpp$")
list(FILTER TachyonSceneSources EXCLUDE REGEX "/core/spec/(serialization|parsing|shapes|schema/migration|template_registry|asset_resolver|pipeline/unified_scene_pipeline)/|legacy")
list(APPEND TachyonSceneSources ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_hash_builder.cpp)

if(TACHYON_ENABLE_LEGACY_JSON_SCENE)
    target_sources(TachyonCore PRIVATE
        core/spec/legacy/scene_spec_json.cpp
        core/spec/legacy/layer_parse_json.cpp
        core/spec/legacy/json_parse_helpers.cpp
        core/spec/property_parse_json.cpp
        core/spec/json_report_utils.cpp
    )
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_LEGACY_JSON_SCENE)
endif()

set(TachyonRuntimeSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/cache_key_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/disk_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/frame_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/compiled_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/property_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/dependency_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_read_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_write_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/compiled_frame_program.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_hasher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/native_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/quality_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_plan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_plan_build.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_plan_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_progress_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_session_audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_session_parallel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_session_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/tile_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_runner_execute.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_runner_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_runner_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_executor_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/animation_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/effect_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/scene_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_job/render_job_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_job/render_job_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_job/render_job_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/audio_preview_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/playback_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/playback_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/resource/render_context.cpp
)

set(TachyonRenderer2DSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/audio/audio_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/backend/cpu_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/backend/vulkan_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/color_management.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/color_management_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/lut3d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/core/framebuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/core/surface_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/deform/mesh_deform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/blur_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_blur_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_color_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_lut_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_misc_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_sampling_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/glitch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/particle_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/utility_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/blur/blur_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color/chromatic_aberration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color/color_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/effect_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/effect_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/utility_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/distort/warp_stabilizer_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/lens_flare_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/particle_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/utility/number_counter_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/matte_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/effects/effect_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/composition_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/feathered_mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/layer_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/layer_to_draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/composition_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_to_draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/feathered_mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/composition_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/raster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/math/math_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/path/shape_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/path/contour/contour_processing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/path/flattening/path_flattening.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/cpu_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mask_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_deform_apply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/perspective_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/sdf_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/fill_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/mask_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_flattener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_trimmer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/stroke_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/precomp_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/render_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/texture_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/spec/gradient_spec_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/spec/project_card.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/freetype/freetype_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/glyph/glyph_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/shaping/font_shaping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/shaping/shaping_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/utf8/utf8_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/text/utils/font_utils.cpp
)

set(TachyonRenderer3DSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/animation/animation3d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/animation/motion_blur.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/core/ray_tracer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/core/ray_tracer_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/core/ray_tracer_shading.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/core/scene_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/effects/camera_cut_timeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/effects/depth_of_field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/materials/material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/materials/material_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/materials/pbr_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/temporal/frame_blend_mode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/temporal/rolling_shutter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/temporal/time_remap_curve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/utilities/debug_tools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/utilities/environment_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/visibility/culling.cpp
)

set(TachyonMediaSources ${TACHYON_ALL_CPP})
list(FILTER TachyonMediaSources INCLUDE REGEX "/media/[^/]+\\.cpp$|/media/ffmpeg/[^/]+\\.cpp$")
list(FILTER TachyonMediaSources EXCLUDE REGEX "/media/(decoding/svg_decoder_impl|decoding/tinygltf_impl|management/asset_manager|management/image_manager|management/media_manager|loading/mesh_loader|streaming/media_prefetcher|resolution/asset_resolution|import/asset_import_pipeline)\\.cpp$")
list(APPEND TachyonMediaSources
    "${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_manifest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/management/proxy_worker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/processing/extruder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_path_utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/resolution/asset_resolution.cpp"
)

set(TachyonAudioSources ${TACHYON_ALL_CPP})
list(FILTER TachyonAudioSources INCLUDE REGEX "/audio/[^/]+\\.cpp$")

set(TachyonOutputSources
    ${CMAKE_CURRENT_SOURCE_DIR}/output/output_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/png_sequence_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/prores_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_command_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_pipe_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output/ffmpeg/ffmpeg_utils.cpp
)

set(TachyonTextSources
    ${CMAKE_CURRENT_SOURCE_DIR}/text/text_measure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/bidi/bidi_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/bidi/script_detector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_coverage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_expression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_paints.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_animator_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_preset_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/animation/text_scene_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/content/subtitle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/content/word_timestamps.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_coverage_reporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_face.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_manifest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_preloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/font_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/core/embedded_fallback_font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/core/font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/core/font_face.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/core/font_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/management/font_downloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/management/font_manifest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/management/font_preloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/management/font_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/utils/font_coverage_reporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/fonts/utils/glyph_atlas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/layout/layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/layout/layout_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/layout/layout_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/layout/layout_engine_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/layout/layout_engine_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/rendering/outline_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/core/rendering/text_raster_surface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text/editing/text_editor.cpp
)

set(TachyonTrackerSources
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/camera_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/feature_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/optical_flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/planar_track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/point_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/rolling_shutter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/stabilizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver_keyframes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/camera_solver_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/corner_detector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/feature_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/image_pyramid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/lk_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/motion_estimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/optical_flow_translation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/planar_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/point_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/rolling_shutter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/stabilizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/algorithms/track_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/planar_track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/planar_track_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tracker/core/track_binding.cpp
)

set(TachyonCLISources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_preview_frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_watch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_fetch_fonts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_transition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/preview_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli.cpp
)

function(tachyon_configure_target target)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb/include>
            $<BUILD_INTERFACE:${tinygltf_SOURCE_DIR}>
            $<BUILD_INTERFACE:${tinyexr_SOURCE_DIR}>
            $<BUILD_INTERFACE:${earcut_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${target}
        PUBLIC
            nlohmann_json::nlohmann_json
            freetype
            harfbuzz
            avcodec
            avformat
            avutil
            swscale
            swresample
            embree
            glfw
            OpenGL::GL
            OpenMP::OpenMP_CXX
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /FS /arch:AVX2)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        target_compile_definitions(${target} PRIVATE TACHYON_AVX2)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    target_compile_definitions(${target} PRIVATE "TACHYON_SFX_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/audio/SoundEffect\"")

    if(WIN32)
        target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
        target_link_libraries(${target} PUBLIC OpenImageDenoise)
    endif()
endfunction()

add_library(TachyonScene OBJECT ${TachyonSceneSources})
tachyon_configure_target(TachyonScene)

add_library(TachyonRuntime OBJECT ${TachyonRuntimeSources})
if(TACHYON_ENABLE_RENDER_JOB_JSON)
    target_sources(TachyonRuntime PRIVATE runtime/execution/render_job/render_job_parse.cpp)
    target_compile_definitions(TachyonRuntime PRIVATE TACHYON_ENABLE_RENDER_JOB_JSON)
endif()
tachyon_configure_target(TachyonRuntime)

add_library(TachyonRenderer2D OBJECT ${TachyonRenderer2DSources})
tachyon_configure_target(TachyonRenderer2D)

add_library(TachyonRenderer3D OBJECT ${TachyonRenderer3DSources})
tachyon_configure_target(TachyonRenderer3D)

add_library(TachyonMedia OBJECT ${TachyonMediaSources})
tachyon_configure_target(TachyonMedia)

add_library(TachyonAudio OBJECT ${TachyonAudioSources})
tachyon_configure_target(TachyonAudio)

add_library(TachyonOutput OBJECT ${TachyonOutputSources})
tachyon_configure_target(TachyonOutput)

add_library(TachyonText OBJECT ${TachyonTextSources})
tachyon_configure_target(TachyonText)

add_library(TachyonTracker OBJECT ${TachyonTrackerSources})
tachyon_configure_target(TachyonTracker)

add_library(TachyonCLI OBJECT ${TachyonCLISources})
tachyon_configure_target(TachyonCLI)

add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_target(TachyonCore)

target_sources(TachyonCore PRIVATE
    $<TARGET_OBJECTS:TachyonScene>
    $<TARGET_OBJECTS:TachyonRuntime>
    $<TARGET_OBJECTS:TachyonRenderer2D>
    $<TARGET_OBJECTS:TachyonRenderer3D>
    $<TARGET_OBJECTS:TachyonMedia>
    $<TARGET_OBJECTS:TachyonAudio>
    $<TARGET_OBJECTS:TachyonOutput>
    $<TARGET_OBJECTS:TachyonText>
    $<TARGET_OBJECTS:TachyonTracker>
    $<TARGET_OBJECTS:TachyonCLI>
)

add_executable(tachyon
    main.cpp
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(tachyon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

target_link_libraries(tachyon
    PRIVATE
        TachyonCore
)

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    add_custom_command(TARGET tachyon POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OIDN_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:tachyon>
        COMMAND_EXPAND_LISTS
    )
endif()
