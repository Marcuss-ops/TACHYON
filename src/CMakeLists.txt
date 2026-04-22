# Core Engine Library
add_library(TachyonCore
    core/cli.cpp
    core/cli/cli_utils.cpp
    core/cli/cli_validate.cpp
    core/cli/cli_inspect.cpp
    core/cli/cli_render.cpp
    core/cli/cli_watch.cpp
    core/options.cpp
    core/report.cpp
    core/camera/camera_state.cpp
    core/expressions/expression_engine.cpp
    core/math/matrix4x4.cpp
    core/math/quaternion.cpp
    core/math/transform3.cpp
    core/model/tachyon.cpp
    audio/audio_analyzer.cpp
    audio/audio_decoder.cpp
    audio/audio_mixer.cpp
    media/image_manager.cpp
    media/asset_resolution.cpp
    media/media_manager.cpp
    media/video_decoder.cpp
    output/ffmpeg/ffmpeg_pipe_sink.cpp
    output/ffmpeg/ffmpeg_command_builder.cpp
    output/ffmpeg/ffmpeg_utils.cpp
    output/ffmpeg/ffmpeg_video_encoder.cpp
    output/png_sequence_sink.cpp
    renderer2d/raster/draw_list_builder.cpp
    renderer2d/raster/draw_list_rasterizer.cpp
    renderer2d/color/blending.cpp
    renderer2d/effects/effect_host.cpp
    renderer2d/effects/effect_utils.cpp
    renderer2d/effects/blur_effects.cpp
    renderer2d/effects/color_effects.cpp
    renderer2d/effects/particle_effects.cpp
    renderer2d/effects/utility_effects.cpp
    renderer2d/evaluated_composition/composition_renderer.cpp
    renderer2d/evaluated_composition/composition_utils.cpp
    renderer2d/evaluated_composition/layer_to_draw_command.cpp
    renderer2d/spec/project_card.cpp
    renderer2d/raster/path/path_flattener.cpp
    renderer2d/raster/path/fill_rasterizer.cpp
    renderer2d/raster/path/stroke_rasterizer.cpp
    renderer2d/raster/path/path_trimmer.cpp
    renderer2d/resource/texture_resolver.cpp
    renderer2d/core/framebuffer.cpp
    renderer2d/core/surface_pool.cpp
    renderer2d/resource/precomp_cache.cpp
    renderer2d/resource/render_context.cpp
    renderer2d/raster/cpu_rasterizer.cpp
    renderer2d/raster/perspective_rasterizer.cpp
    runtime/execution/batch_runner/batch_runner_parse.cpp
    runtime/execution/batch_runner/batch_runner_validate.cpp
    runtime/execution/batch_runner/batch_runner_execute.cpp
    runtime/execution/batch_runner/batch_runner_utils.cpp
    runtime/cache/frame_cache.cpp
    runtime/cache/cache_key_builder.cpp
    runtime/execution/frame_executor.cpp
    runtime/execution/frame_executor/frame_executor_utils.cpp
    runtime/execution/frame_executor/frame_evaluator_node.cpp
    runtime/execution/frame_executor/frame_evaluator_property.cpp
    runtime/execution/frame_executor/frame_evaluator_layer.cpp
    runtime/execution/frame_executor/frame_evaluator_composition.cpp
    runtime/execution/quality_policy.cpp
    runtime/core/render_graph.cpp
    runtime/core/property_graph.cpp
    runtime/core/compiled_scene.cpp
    runtime/core/tbf_codec.cpp
    runtime/execution/render_session.cpp
    runtime/execution/tile_scheduler.cpp
    runtime/execution/render_job/render_job_parse.cpp
    runtime/execution/render_job/render_job_validate.cpp
    runtime/execution/render_job/render_job_utils.cpp
    runtime/execution/render_plan.cpp
    runtime/execution/frame_hasher.cpp
    runtime/resource/render_context.cpp
    core/scene/evaluator.cpp
    core/scene/evaluator/hashing.cpp
    core/scene/evaluator/templates.cpp
    core/scene/evaluator/property_sampler.cpp
    core/scene/evaluator/layer_utils.cpp
    core/scene/evaluator/mesh_animator.cpp
    core/scene/evaluator/light_evaluator.cpp
    core/scene/evaluator/layer_evaluator.cpp
    core/scene/evaluator_composition.cpp
    core/spec/json_parse_helpers.cpp
    core/spec/layer_parse_json.cpp
    core/spec/property_parse_json.cpp
    core/spec/scene_compiler.cpp

    core/scene/evaluator_math.cpp
    core/spec/scene_spec.cpp
    core/spec/scene_spec_core.cpp
    core/spec/scene_spec_json.cpp
    core/spec/scene_spec_builder.cpp
    renderer2d/evaluated_composition/layer_renderer.cpp
    renderer2d/evaluated_composition/mask_renderer.cpp
    renderer2d/evaluated_composition/effect_renderer.cpp
    renderer2d/evaluated_composition/render_graph.cpp
#   renderer2d/evaluated_composition/raster.cpp
    text/font.cpp
    text/font_registry.cpp
    text/text_raster_surface.cpp
    text/text_animator_utils.cpp
    text/layout.cpp
    text/layout_engine.cpp
    text/subtitle.cpp
    timeline/frame_evaluator.cpp
    timeline/time.cpp
    timeline/time_resolver.cpp
    renderer2d/color/lut3d.cpp
    renderer2d/math/math_utils.cpp
    renderer2d/audio/audio_sampling.cpp
    renderer2d/text/utf8/utf8_decoder.cpp
    renderer2d/text/shaping/font_shaping.cpp
    renderer2d/text/utils/font_utils.cpp
    renderer2d/text/freetype/freetype_manager.cpp
    renderer2d/text/glyph/glyph_loader.cpp
    text/outline_extractor.cpp
    renderer3d/ray_tracer.cpp
    renderer3d/ray_tracer_shading.cpp
    renderer3d/ray_tracer_scene.cpp
    renderer3d/pbr_utils.cpp
    renderer3d/environment_manager.cpp
    media/mesh_loader.cpp
    media/extruder.cpp
)

if(MSVC)
    target_compile_options(TachyonCore PRIVATE /W4 /WX /permissive- /utf-8 /FS)
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
