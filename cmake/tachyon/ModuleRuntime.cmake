# TachyonRuntime sources and targets
# This file is consolidated for robust static linking

set(TachyonRuntimeCompilerSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/component_library.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/layer_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/preset_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/property_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler_build.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler_hashing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler_property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_compiler_resolve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/compiler/scene_hash_builder.cpp
)

set(TachyonRuntimeCoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime_facade.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/compiled_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/data/property_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/dependency_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/graph/render_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_read_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/core/serialization/tbf_write_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/cache_key_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/disk_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/frame_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/resource/render_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/worker_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/surface_pool_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/telemetry_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/diagnostics/report.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/server.cpp
)


set(TachyonRuntimeExecutionSources
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
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/parallel/taskflow_runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_session_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_runner_execute.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_runner_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/batch_runner/batch_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluator_property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_blend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_cache_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_time_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_evaluation_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_pipeline_steps.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/motion_blur_sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_fallback_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/rasterization_step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/frame_executor/frame_executor_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/animation_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/effect_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/planning/hash/scene_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_job/render_job_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/render_job/render_job_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/property_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/execution/tile_scheduler.cpp
)

set(TachyonRuntimePlaybackSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/audio_preview_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/playback_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/playback/playback_queue.cpp
)

set(TachyonRuntimeProfilingSources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/profiling/render_profiler.cpp
)

set(TachyonRuntimeTelemetrySources
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/telemetry/render_telemetry_record.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/telemetry/process_resource_sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/telemetry/telemetry_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/telemetry/batch_telemetry_aggregator.cpp
)

set(TachyonRuntimeSources
    ${TachyonRuntimeCoreSources}
    ${TachyonRuntimeCompilerSources}
    ${TachyonRuntimeExecutionSources}
    ${TachyonRuntimePlaybackSources}
    ${TachyonRuntimeProfilingSources}
    ${TachyonRuntimeTelemetrySources}
)

# --- Consolidated Target ---

add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)

if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonRuntime PRIVATE TACHYON_ENABLE_AUDIO_MUX)
endif()

target_link_libraries(TachyonRuntime
    PUBLIC
        TachyonCore
        TachyonColor
        TachyonScene
        TachyonRenderer2D
        TachyonOutput
    PRIVATE
        TachyonPlatform
        TachyonText
)

if(TACHYON_ENABLE_MEDIA)
    target_link_libraries(TachyonRuntime PUBLIC TachyonMedia)
endif()

if(TACHYON_ENABLE_AUDIO_MUX)
    target_link_libraries(TachyonRuntime PRIVATE TachyonAudio)
endif()

tachyon_link_text_deps(TachyonRuntime)
tachyon_link_media_deps(TachyonRuntime)
tachyon_link_output_deps(TachyonRuntime)
tachyon_link_omp(TachyonRuntime)

if(TACHYON_ENABLE_TASKFLOW)
    target_compile_definitions(TachyonRuntime PUBLIC TACHYON_ENABLE_TASKFLOW)
    if(TARGET Taskflow)
        target_link_libraries(TachyonRuntime PUBLIC Taskflow)
    elseif(TARGET Taskflow::Taskflow)
        target_link_libraries(TachyonRuntime PUBLIC Taskflow::Taskflow)
    elseif(TARGET taskflow)
        target_link_libraries(TachyonRuntime PUBLIC taskflow)
    endif()
endif()

# Legacy compatibility targets
add_library(TachyonRuntimeCore INTERFACE)
target_link_libraries(TachyonRuntimeCore INTERFACE TachyonRuntime)

add_library(TachyonRuntimeExecution INTERFACE)
target_link_libraries(TachyonRuntimeExecution INTERFACE TachyonRuntime)

add_library(TachyonRuntimeCompiler INTERFACE)
target_link_libraries(TachyonRuntimeCompiler INTERFACE TachyonRuntime)

add_library(TachyonRuntimeTelemetry INTERFACE)
target_link_libraries(TachyonRuntimeTelemetry INTERFACE TachyonRuntime)

add_library(TachyonRuntimePlayback INTERFACE)
target_link_libraries(TachyonRuntimePlayback INTERFACE TachyonRuntime)

add_library(TachyonRuntimeEngine INTERFACE)
target_link_libraries(TachyonRuntimeEngine INTERFACE TachyonRuntime)
