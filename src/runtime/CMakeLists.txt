# Tachyon Runtime Module

set(TachyonRuntimeSources
    runtime_facade.cpp
    
    cache/cache_key_builder.cpp
    cache/disk_cache.cpp
    cache/frame_cache.cpp
    cache/node_cache.cpp
    
    compiler/component_library.cpp
    compiler/layer_compiler.cpp
    compiler/preset_compiler.cpp
    compiler/property_compiler.cpp
    compiler/scene_compiler.cpp
    compiler/scene_compiler_build.cpp
    compiler/scene_compiler_graph.cpp
    compiler/scene_compiler_hashing.cpp
    compiler/scene_compiler_property.cpp
    compiler/scene_compiler_resolve.cpp
    compiler/scene_hash_builder.cpp
    
    core/data/compiled_scene.cpp
    core/data/property_graph.cpp
    core/graph/dependency_graph.cpp
    core/graph/render_graph.cpp
    core/serialization/tbf_codec.cpp
    
    diagnostics/report.cpp
    
    execution/batch_runner/batch_registry.cpp
    execution/batch_runner/batch_runner_execute.cpp
    execution/batch_runner/batch_runner_validate.cpp
    execution/compiled_frame_program.cpp
    execution/frame_executor.cpp
    execution/frame_executor/frame_blend.cpp
    execution/frame_executor/frame_cache_policy.cpp
    execution/frame_executor/frame_evaluation_pipeline.cpp
    execution/frame_executor/frame_evaluator_composition.cpp
    execution/frame_executor/frame_evaluator_layer.cpp
    execution/frame_executor/frame_evaluator_node.cpp
    execution/frame_executor/frame_evaluator_property.cpp
    execution/frame_executor/frame_executor_utils.cpp
    execution/frame_executor/frame_fallback_policy.cpp
    execution/frame_executor/frame_pipeline_steps.cpp
    execution/frame_executor/frame_time_resolver.cpp
    execution/frame_executor/motion_blur_sampler.cpp
    execution/frame_executor/rasterization_step.cpp
    execution/frame_hasher.cpp
    execution/native_render.cpp
    execution/parallel/taskflow_runtime.cpp
    execution/planning/hash/animation_hash.cpp
    execution/planning/hash/effect_hash.cpp
    execution/planning/hash/scene_hash.cpp
    execution/planning/render_preflight.cpp
    execution/property_sampling.cpp
    execution/quality_policy.cpp
    execution/render_job/render_job_builder.cpp
    execution/render_job/render_job_validate.cpp
    execution/render_plan.cpp
    execution/render_plan_build.cpp
    execution/render_plan_hash.cpp
    execution/render_progress_sink.cpp
    execution/render_session.cpp
    execution/render_session_audio.cpp
    execution/render_session_parallel.cpp
    execution/session/render_preflight.cpp
    execution/tile_scheduler.cpp
    
    media/media_pipeline.cpp
    media/probe_cache.cpp
    media/management/media_manager.cpp
    media/management/image_manager.cpp
    media/management/proxy_manifest.cpp
    media/management/proxy_worker.cpp
    media/management/instrumentation.cpp
    media/management/asset_manager.cpp
    media/import/asset_import_pipeline.cpp
    media/streaming/media_prefetcher.cpp
    media/resolution/path_resolver.cpp
    media/resolution/asset_resolver.cpp
    
    transitions/transition_ops.cpp
    
    playback/audio_preview_buffer.cpp
    playback/playback_engine.cpp
    playback/playback_queue.cpp
    
    policy/surface_pool_policy.cpp
    policy/telemetry_policy.cpp
    policy/worker_policy.cpp
    
    profiling/render_profiler.cpp
    
    registry/engine_registry.cpp
    
    resource/render_context.cpp
    resource/surface_pool.cpp
    
    telemetry/batch_telemetry_aggregator.cpp
    telemetry/process_resource_sampler.cpp
    telemetry/render_telemetry_record.cpp
    telemetry/telemetry_writer.cpp
    telemetry/sqlite_telemetry_store.cpp
)

add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
tachyon_link_taskflow(TachyonRuntime)
tachyon_link_mimalloc(TachyonRuntime)
tachyon_link_infra(TachyonRuntime)

target_link_libraries(TachyonRuntime
    PUBLIC
        TachyonScene
        TachyonRenderer2D
        TachyonBackends
        TachyonOutput
        TachyonOperations
    PRIVATE
        TachyonPlatform
)

if(TACHYON_ENABLE_SQLITE_TELEMETRY)
    target_link_libraries(TachyonRuntime PRIVATE tachyon_sqlite)
    target_compile_definitions(TachyonRuntime PUBLIC TACHYON_ENABLE_SQLITE_TELEMETRY)
endif()

