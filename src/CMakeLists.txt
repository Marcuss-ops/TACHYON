# Core Engine Library
add_library(TachyonCore
    runtime/execution/frame_hasher.cpp
    runtime/execution/quality_policy.cpp
    runtime/execution/tile_scheduler.cpp
    runtime/execution/frame_executor.cpp
    runtime/execution/render_plan.cpp
    runtime/execution/frame_executor/frame_executor_utils.cpp
    runtime/execution/frame_executor/frame_evaluator_node.cpp
    runtime/execution/frame_executor/frame_evaluator_property.cpp
    runtime/execution/frame_executor/frame_evaluator_layer.cpp
    runtime/execution/frame_executor/frame_evaluator_composition.cpp
    runtime/execution/render_job/render_job_parse.cpp
    runtime/execution/render_job/render_job_validate.cpp
    runtime/execution/render_job/render_job_utils.cpp
    runtime/execution/batch_runner/batch_runner_execute.cpp
    runtime/execution/batch_runner/batch_runner_parse.cpp
    runtime/execution/batch_runner/batch_runner_utils.cpp
    runtime/execution/batch_runner/batch_runner_validate.cpp
    runtime/cache/cache_key_builder.cpp
    runtime/cache/frame_cache.cpp
    runtime/resource/render_context.cpp
    runtime/playback/playback_queue.cpp
    runtime/playback/playback_engine.cpp
    runtime/playback/audio_preview_buffer.cpp
    runtime/execution/render_session.cpp
    runtime/core/graph/render_graph.cpp
    runtime/core/data/property_graph.cpp
    runtime/core/data/compiled_scene.cpp
    runtime/cache/disk_cache.cpp
    core/cli.cpp
    core/cli/cli_utils.cpp
    core/cli/cli_validate.cpp
    core/cli/cli_inspect.cpp
    core/cli/cli_render.cpp
    core/cli/cli_watch.cpp
    core/options.cpp
    core/report.cpp
    core/camera/camera_state.cpp
    core/camera/camera_shake.cpp
    core/expressions/expression_engine.cpp
    core/expressions/expression_vm.cpp
    core/math/matrix4x4.cpp
    core/math/quaternion.cpp
    core/math/transform3.cpp
    core/properties/bezier_interpolator.cpp
    core/model/tachyon.cpp
    core/scene/evaluator.cpp
    core/scene/evaluator_composition.cpp
    core/scene/evaluator_math.cpp
    core/scene/evaluator/hashing.cpp
    core/scene/evaluator/templates.cpp
    core/scene/evaluator/property_sampler.cpp
    core/scene/evaluator/layer_utils.cpp
    core/scene/evaluator/mesh_animator.cpp
    core/scene/evaluator/light_evaluator.cpp
    core/scene/evaluator/layer_evaluator.cpp
    core/scene/rigging/rig_graph.cpp
    core/scene/rigging/ik_solver.cpp
    core/scene/rigging/constraint_solver.cpp
    core/spec/json_parse_helpers.cpp
    core/spec/layer_parse_json.cpp
    core/spec/property_parse_json.cpp
    core/spec/compilation/scene_compiler.cpp
    core/spec/schema/scene_spec.cpp
    core/spec/schema/scene_spec_core.cpp
    core/spec/scene_spec_json.cpp
    core/spec/scene_spec_builder.cpp
    core/spec/scene_spec_audio.cpp
    renderer2d/raster/draw_list_builder.cpp
    renderer2d/raster/draw_list_rasterizer.cpp
    renderer2d/color/blending.cpp
    renderer2d/color/lut3d.cpp
    renderer2d/color/color_management.cpp
    renderer2d/color/color_management_system.cpp
    renderer2d/effects/effect_host.cpp
    renderer2d/effects/effect_utils.cpp
    renderer2d/effects/blur_effects.cpp
    renderer2d/effects/color_effects.cpp
    renderer2d/effects/particle_effects.cpp
    renderer2d/effects/utility_effects.cpp
    renderer2d/backend/cpu_backend.cpp
    renderer2d/evaluated_composition/matte_resolver.cpp
    renderer2d/evaluated_composition/rendering/composition_renderer.cpp
    renderer2d/evaluated_composition/utilities/composition_utils.cpp
    renderer2d/evaluated_composition/rendering/layer_to_draw_command.cpp
    renderer2d/evaluated_composition/rendering/mask_renderer.cpp
    renderer2d/evaluated_composition/effects/effect_renderer.cpp
    renderer2d/evaluated_composition/rendering/layer_renderer.cpp
    renderer2d/evaluated_composition/utilities/render_graph.cpp
    renderer2d/spec/project_card.cpp
    renderer2d/raster/path/path_flattener.cpp
    renderer2d/raster/path/fill_rasterizer.cpp
    renderer2d/raster/path/stroke_rasterizer.cpp
    renderer2d/raster/mask_rasterizer.cpp
    renderer2d/raster/path/path_trimmer.cpp
    renderer2d/raster/cpu_rasterizer.cpp
    renderer2d/raster/perspective_rasterizer.cpp
    renderer2d/resource/texture_resolver.cpp
    renderer2d/core/framebuffer.cpp
    renderer2d/core/surface_pool.cpp
    renderer2d/resource/precomp_cache.cpp
    renderer2d/resource/render_context.cpp
    renderer2d/math/math_utils.cpp
    renderer2d/audio/audio_sampling.cpp
    renderer2d/text/utf8/utf8_decoder.cpp
    renderer2d/text/glyph/glyph_loader.cpp
    renderer2d/text/utils/font_utils.cpp
    renderer2d/text/freetype/freetype_manager.cpp
    renderer2d/text/shaping/font_shaping.cpp
    renderer2d/text/shaping/shaping_cache.cpp
    audio/audio_analyzer.cpp
    audio/audio_decoder.cpp
    audio/audio_mixer.cpp
    audio/audio_graph.cpp
    audio/audio_processor.cpp
    audio/audio_encoder.cpp
    audio/audio_export.cpp
    media/image_manager.cpp
    media/asset_resolution.cpp
    media/media_manager.cpp
    media/video_decoder.cpp
    media/media_prefetcher.cpp
    media/management/proxy_manifest.cpp
    media/management/proxy_worker.cpp
    media/mesh_loader.cpp
    media/extruder.cpp
    output/ffmpeg/ffmpeg_pipe_sink.cpp
    output/ffmpeg/ffmpeg_command_builder.cpp
    output/ffmpeg/ffmpeg_utils.cpp
    output/ffmpeg/ffmpeg_video_encoder.cpp
    output/png_sequence_sink.cpp
    timeline/frame_evaluator.cpp
    timeline/time.cpp
    timeline/time_resolver.cpp
    timeline/time_remap.cpp
    timeline/frame_blend.cpp
    timeline/camera_cuts.cpp
    text/bidi/bidi_engine.cpp
    text/core/animation/text_animator_utils.cpp
    text/core/fonts/font.cpp
    text/core/fonts/font_registry.cpp
    text/core/layout/layout.cpp
    text/core/layout/layout_engine.cpp
    text/core/rendering/outline_extractor.cpp
    text/bidi/script_detector.cpp
    text/core/content/subtitle.cpp
    text/core/animation/text_animator_utils.cpp
    text/editing/text_editor.cpp
    text/core/rendering/text_raster_surface.cpp
    tracker/feature_tracker.cpp
    tracker/track.cpp
    tracker/track_binding.cpp
    renderer3d/core/ray_tracer.cpp
    renderer3d/core/scene_cache.cpp
    renderer3d/animation/motion_blur.cpp
    renderer3d/animation/animation3d.cpp
    renderer3d/effects/depth_of_field.cpp
    renderer3d/materials/material.cpp
    renderer3d/materials/material_evaluator.cpp
    renderer3d/visibility/culling.cpp
    renderer3d/utilities/debug_tools.cpp
    renderer3d/materials/pbr_utils.cpp
    renderer3d/utilities/environment_manager.cpp
    importer/usd_importer.cpp
    importer/alembic_importer.cpp
    camera_cut_contract.cpp
)

if(MSVC)
    target_compile_options(TachyonCore PRIVATE /W4 /WX /permissive- /utf-8 /FS /arch:AVX2)
    target_compile_definitions(TachyonCore PRIVATE TACHYON_AVX2)
else()
    target_compile_options(TachyonCore PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(TachyonCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb/include>
        $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(TachyonCore
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
        OpenImageDenoise
        OpenMP::OpenMP_CXX
)

# --- Optional dependencies ---

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    message(STATUS "[Tachyon] Vulkan found – enabling GPU compute backend")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_VULKAN)
    target_link_libraries(TachyonCore PUBLIC Vulkan::Vulkan)
else()
    message(STATUS "[Tachyon] Vulkan NOT found – CPU backend only")
endif()

find_package(OpenColorIO QUIET)
if(OpenColorIO_FOUND)
    message(STATUS "[Tachyon] OpenColorIO found – enabling OCIO adapter")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_OCIO)
    target_link_libraries(TachyonCore PUBLIC OpenColorIO::OpenColorIO)
else()
    message(STATUS "[Tachyon] OpenColorIO NOT found – using built-in colour pipeline")
endif()

find_package(OpenEXR QUIET)
if(OpenEXR_FOUND)
    message(STATUS "[Tachyon] OpenEXR found – enabling EXR sequence sink")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_EXR)
    target_link_libraries(TachyonCore PUBLIC OpenEXR::OpenEXR)
    target_sources(TachyonCore PRIVATE output/exr_sequence_sink.cpp)
else()
    message(STATUS "[Tachyon] OpenEXR NOT found – EXR output disabled")
endif()

find_package(pxr QUIET)
if(pxr_FOUND)
    message(STATUS "[Tachyon] OpenUSD found – enabling USD scene importer")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_USD)
    target_link_libraries(TachyonCore PUBLIC usd_ms)
else()
    message(STATUS "[Tachyon] OpenUSD NOT found – USD import disabled")
endif()

find_package(Alembic QUIET)
if(Alembic_FOUND)
    message(STATUS "[Tachyon] Alembic found – enabling Alembic scene importer")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ALEMBIC)
    target_link_libraries(TachyonCore PUBLIC Alembic::Alembic)
else()
    message(STATUS "[Tachyon] Alembic NOT found – Alembic import disabled")
endif()

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

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
