# Core Engine Library (Shared for C API / DLL)
file(GLOB _tachyon_core_spec_parsing_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/*.cpp"
)
file(GLOB _tachyon_core_spec_compilation_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/*.cpp"
)
list(REMOVE_ITEM _tachyon_core_spec_compilation_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_hashing.cpp"
)
file(GLOB _tachyon_runtime_serialization_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/*.cpp"
)
file(GLOB _tachyon_runtime_hash_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/*.cpp"
)

add_library(TachyonCore SHARED
    runtime/execution/frame_hasher.cpp
    runtime/execution/quality_policy.cpp
    runtime/execution/tile_scheduler.cpp
    runtime/execution/frame_executor.cpp
    runtime/execution/render_plan_hash.cpp
    runtime/execution/render_plan_build.cpp
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
     runtime/execution/render_session_parallel.cpp
     runtime/execution/render_session_serialization.cpp
     runtime/execution/render_session_audio.cpp
    core/c_api.cpp
    core/transition_registry.cpp
    core/animation_primitives.cpp
    runtime/core/graph/render_graph.cpp
    runtime/core/data/property_graph.cpp
    runtime/core/data/compiled_scene.cpp
    runtime/cache/disk_cache.cpp
    core/cli.cpp
    core/cli/cli_utils.cpp
    core/cli/cli_validate.cpp
    core/cli/cli_inspect.cpp
    core/cli/cli_render.cpp
    core/cli/cli_demo.cpp
    core/cli/cli_preview_frame.cpp
    core/cli/cli_transition.cpp
    core/cli/cli_watch.cpp
    core/options.cpp
    core/report.cpp
    core/camera/camera_state.cpp
    core/camera/camera_shake.cpp
    core/scene/state/evaluated_camera2d_state.cpp
    core/expressions/expression_engine.cpp
    core/expressions/expression_vm.cpp
    core/math/matrix4x4.cpp
    core/math/quaternion.cpp
    core/math/transform3.cpp
    core/math/vector2_serialize.cpp
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
    background_generator.cpp
    core/scene/evaluator/light_evaluator.cpp
    core/scene/evaluator/camera2d_evaluator.cpp
    core/scene/evaluator/layer_evaluator.cpp
    core/scene/rigging/rig_graph.cpp
    core/scene/rigging/ik_solver.cpp
    core/scene/rigging/constraint_solver.cpp
    core/scene/dependency_node.cpp
    core/spec/json_parse_helpers.cpp
    core/spec/layer_parse_json.cpp
    core/spec/property_parse_json.cpp
    core/spec/serialization/property_scalar_serialize.cpp
    core/spec/serialization/property_vector_serialize.cpp
    core/spec/serialization/property_color_serialize.cpp
    core/spec/text_animator_spec_serialize.cpp
    core/spec/schema/common/common_spec_serialize.cpp
    core/spec/schema/objects/layer_spec_serialize.cpp
    core/shapes/shape_modifiers.cpp
    core/spec/schema/scene_spec.cpp
    core/spec/schema/scene_spec_core.cpp
    core/spec/serialization/layer_serialize.cpp
    core/spec/scene_spec_serialize.cpp
    core/spec/scene_spec_parse.cpp
    core/spec/scene_spec_builder.cpp
    core/spec/template_registry.cpp
    core/spec/validation/scene_validator_layer.cpp
    core/spec/validation/scene_validator_schema.cpp
    core/spec/validation/scene_validator_utils.cpp
    core/spec/validation/scene_validator_composition.cpp
    core/spec/scene_spec_audio.cpp
    renderer2d/raster/draw_list_builder.cpp
    renderer2d/raster/draw_list_rasterizer.cpp
    renderer2d/spec/gradient_spec_serialize.cpp
     renderer2d/color/blending.cpp
     renderer2d/color/blending_parse.cpp
     renderer2d/color/blending_math.cpp
    renderer2d/color/lut3d.cpp
    renderer2d/color/color_management.cpp
    renderer2d/color/color_management_system.cpp
    renderer2d/effects/core/effect_host.cpp
    renderer2d/effects/core/glsl_transition_effect.cpp
    renderer2d/effects/core/effect_utils.cpp
    renderer2d/effects/blur/blur_effects.cpp
    renderer2d/effects/color/color_effects.cpp
    renderer2d/effects/color/chromatic_aberration.cpp
    renderer2d/effects/generators/particle_effects.cpp
    renderer2d/effects/generators/lens_flare_effect.cpp
    renderer2d/effects/core/utility_effects.cpp
    renderer2d/effects/distort/glitch.cpp
    renderer2d/effects/distort/warp_stabilizer_effect.cpp
    renderer2d/effects/utility/number_counter_effect.cpp
    renderer2d/backend/cpu_backend.cpp
    renderer2d/evaluated_composition/matte_resolver.cpp
    renderer2d/evaluated_composition/rendering/pipeline/composition_renderer.cpp
    renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.cpp
    renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.cpp
    renderer2d/evaluated_composition/rendering/primitives/layer_renderer_simple.cpp
    renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.cpp
    renderer2d/evaluated_composition/utilities/composition_utils.cpp
    renderer2d/evaluated_composition/rendering/pipeline/layer_to_draw_command.cpp
    renderer2d/evaluated_composition/rendering/primitives/mask_renderer.cpp
    renderer2d/evaluated_composition/effects/effect_renderer.cpp
    renderer2d/evaluated_composition/rendering/pipeline/layer_renderer.cpp
    renderer2d/evaluated_composition/utilities/render_graph.cpp
    renderer2d/spec/project_card.cpp
    renderer2d/raster/path/path_flattener.cpp
    renderer2d/raster/path/fill_rasterizer.cpp
    renderer2d/raster/path/stroke_rasterizer.cpp
    renderer2d/raster/mask_rasterizer.cpp
    renderer2d/raster/mesh_rasterizer.cpp
    renderer2d/raster/mesh_deform_apply.cpp
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
    audio/processing/audio_analyzer.cpp
    audio/io/audio_decoder.cpp
    audio/core/audio_mixer.cpp
    audio/core/audio_graph.cpp
    audio/processing/audio_processor.cpp
    audio/io/audio_encoder.cpp
    audio/io/audio_export.cpp
    audio/processing/loudness_meter.cpp
    audio/processing/waveform.cpp
    tracker/algorithms/optical_flow.cpp
    tracker/algorithms/optical_flow_math.cpp
    tracker/algorithms/optical_flow_translation.cpp
    tracker/algorithms/camera_solver.cpp
    tracker/algorithms/camera_solver_keyframes.cpp
    tracker/algorithms/camera_solver_math.cpp
    tracker/algorithms/feature_tracker.cpp
    tracker/algorithms/corner_detector.cpp
    tracker/algorithms/image_pyramid.cpp
    tracker/algorithms/lk_tracker.cpp
    tracker/algorithms/motion_estimation.cpp
    tracker/algorithms/track_utils.cpp
    tracker/algorithms/planar_tracker.cpp
    tracker/algorithms/point_tracker.cpp
    tracker/algorithms/rolling_shutter.cpp
    tracker/algorithms/stabilizer.cpp
    tracker/core/track.cpp
    tracker/core/track_binding.cpp
    tracker/core/planar_track.cpp
    tracker/core/planar_track_binding.cpp
    media/management/image_manager.cpp
    media/management/asset_manager.cpp
    media/resolution/asset_resolution.cpp
    media/management/media_manager.cpp
    media/decoding/video_decoder.cpp
    media/streaming/media_prefetcher.cpp
    media/management/proxy_manifest.cpp
    media/management/proxy_worker.cpp
    media/loading/mesh_loader.cpp
    media/processing/extruder.cpp
    media/decoding/stb_image_impl.cpp
     media/decoding/tinygltf_impl.cpp
     media/decoding/svg_decoder_impl.cpp
    output/ffmpeg/ffmpeg_pipe_sink.cpp
    output/ffmpeg/ffmpeg_command_builder.cpp
    output/ffmpeg/ffmpeg_utils.cpp
    output/ffmpeg/ffmpeg_video_encoder.cpp
     output/png_sequence_sink.cpp
     output/prores_sink.cpp
    timeline/frame_evaluator.cpp
    timeline/time.cpp
    timeline/time_resolver.cpp
    timeline/time_remap.cpp
    timeline/frame_blend.cpp
    text/bidi/bidi_engine.cpp
    text/core/animation/text_animator_utils.cpp
    text/core/animation/text_animator_paints.cpp
    text/core/animation/text_animator_sampling.cpp
    text/core/animation/text_animator_context.cpp
    text/core/animation/text_animator_coverage.cpp
    text/core/animation/text_presets.cpp
    text/core/fonts/core/font.cpp
    text/core/fonts/core/font_registry.cpp
    text/core/fonts/core/font_face.cpp
    text/core/fonts/core/font_instance.cpp
    text/core/fonts/utils/glyph_atlas.cpp
    text/core/fonts/management/font_manifest.cpp
    text/core/fonts/management/font_resolver.cpp
    text/core/fonts/management/font_preloader.cpp
    text/core/fonts/utils/font_coverage_reporter.cpp
     text/core/layout/layout.cpp
     text/core/layout/layout_engine.cpp
     text/core/layout/layout_engine_helpers.cpp
     text/core/layout/layout_engine_core.cpp
     text/core/layout/layout_cache.cpp
    text/core/rendering/outline_extractor.cpp
    text/bidi/script_detector.cpp
    text/core/content/subtitle.cpp
    text/core/content/word_timestamps.cpp
    text/editing/text_editor.cpp
    text/text_measure.cpp
    text/core/animation/text_animator_context.cpp
    text/core/animation/text_animator_coverage.cpp
    text/core/animation/text_animator_expression.cpp
    text/core/animation/text_animator_paints.cpp
    text/core/animation/text_animator_sampling.cpp
    text/core/animation/text_animator_utils.cpp
    text/core/animation/text_presets.cpp
    text/bidi/bidi_engine.cpp
    renderer3d/temporal/time_remap_curve.cpp
    renderer3d/temporal/frame_blend_mode.cpp
    renderer3d/temporal/rolling_shutter.cpp
    text/core/rendering/text_raster_surface.cpp
    studio/studio_library.cpp
    renderer3d/core/ray_tracer.cpp
    renderer3d/core/ray_tracer_shading.cpp
    renderer3d/core/ray_tracer_scene.cpp
    renderer3d/core/scene_cache.cpp
    renderer3d/animation/motion_blur.cpp
    renderer3d/animation/animation3d.cpp
    renderer3d/effects/depth_of_field.cpp
    renderer3d/materials/material.cpp
    renderer3d/materials/material_evaluator.cpp
    renderer3d/materials/pbr_utils.cpp
    renderer3d/visibility/culling.cpp
    renderer3d/utilities/debug_tools.cpp
    renderer3d/utilities/environment_manager.cpp
    importer/usd_importer.cpp
    importer/alembic_importer.cpp
    camera_cut_contract.cpp
    editor/undo/undo_manager.cpp
    editor/autosave/autosave_manager.cpp
    debug/debug_export.cpp
    debug/crash_handler.cpp
    ${_tachyon_core_spec_parsing_sources}
    ${_tachyon_core_spec_compilation_sources}
    ${_tachyon_runtime_serialization_sources}
    ${_tachyon_runtime_hash_sources}
)

file(GLOB _tachyon_unity_excluded_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/blur/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/distort/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/utility/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer3d/core/*.cpp"
)

list(APPEND _tachyon_unity_excluded_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mask_rasterizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/media/tinygltf_impl.cpp"
)

target_compile_definitions(TachyonCore
    PRIVATE
        TACHYON_SHARED
        TACHYON_EXPORTS
        TACHYON_BUILD_DLL
    PUBLIC
        TACHYON_USE_DLL
)

if(MSVC)
    set_target_properties(TachyonCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

foreach(_source IN LISTS _tachyon_unity_excluded_sources)
    set_source_files_properties(${_source} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
endforeach()

if(MSVC)
    target_compile_options(TachyonCore PRIVATE
        /W4 /EHsc /permissive- /utf-8 /FS /arch:AVX2 /MP
        $<$<CONFIG:Release>:/O2>
        $<$<BOOL:${TACHYON_TREAT_WARNINGS_AS_ERRORS}>:/WX>
    )
    target_compile_definitions(TachyonCore PRIVATE TACHYON_AVX2)
else()
    target_compile_options(TachyonCore PRIVATE -Wall -Wextra -Wpedantic
        $<$<BOOL:${TACHYON_TREAT_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

target_include_directories(TachyonCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_include_directories(TachyonCore PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
endif()

# FFmpeg libraries (optional)
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)
find_library(SWRESAMPLE_LIBRARY swresample)

target_link_libraries(TachyonCore
    PUBLIC
        nlohmann_json::nlohmann_json
        freetype
        harfbuzz
        OpenMP::OpenMP_CXX
        spdlog::spdlog
        pugixml
)

target_sources(TachyonCore PRIVATE
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.engine.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.offset.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.rectclip.cpp
)

if(TACHYON_ENABLE_TRACY)
    target_link_libraries(TachyonCore PUBLIC TracyClient)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_TRACY_ENABLED)
endif()

if(AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY AND SWRESAMPLE_LIBRARY)
    message(STATUS "[Tachyon] FFmpeg found - enabling video/audio support")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_HAS_FFMPEG)
    target_link_libraries(TachyonCore PUBLIC
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
    )
else()
    message(STATUS "[Tachyon] FFmpeg NOT found - media features disabled")
endif()

if(TARGET embree)
    target_link_libraries(TachyonCore PUBLIC embree)
endif()

if(WIN32)
    target_link_libraries(TachyonCore PUBLIC OpenImageDenoise)
endif()

# --- Optional dependencies ---

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    message(STATUS "[Tachyon] Vulkan found - enabling GPU compute backend")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_VULKAN)
    target_link_libraries(TachyonCore PUBLIC Vulkan::Vulkan)
else()
    message(STATUS "[Tachyon] Vulkan NOT found - CPU backend only")
endif()

find_package(OpenColorIO QUIET)
if(OpenColorIO_FOUND)
    message(STATUS "[Tachyon] OpenColorIO found - enabling OCIO adapter")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_OCIO)
    target_link_libraries(TachyonCore PUBLIC OpenColorIO::OpenColorIO)
else()
    message(STATUS "[Tachyon] OpenColorIO NOT found - using built-in colour pipeline")
endif()

find_package(OpenEXR QUIET)
if(OpenEXR_FOUND)
    message(STATUS "[Tachyon] OpenEXR found - enabling EXR sequence sink")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_EXR)
    target_link_libraries(TachyonCore PUBLIC OpenEXR::OpenEXR)
    target_sources(TachyonCore PRIVATE output/exr_sequence_sink.cpp)
else()
    message(STATUS "[Tachyon] OpenEXR NOT found - EXR output disabled")
endif()

find_package(pxr QUIET)
if(pxr_FOUND)
    message(STATUS "[Tachyon] OpenUSD found - enabling USD scene importer")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_USD)
    target_link_libraries(TachyonCore PUBLIC usd_ms)
else()
    message(STATUS "[Tachyon] OpenUSD NOT found - USD import disabled")
endif()

find_package(Alembic QUIET)
if(Alembic_FOUND)
    message(STATUS "[Tachyon] Alembic found - enabling Alembic scene importer")
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ALEMBIC)
    target_link_libraries(TachyonCore PUBLIC Alembic::Alembic)
else()
    message(STATUS "[Tachyon] Alembic NOT found - Alembic import disabled")
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
