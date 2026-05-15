# Tachyon Runtime Module

set(TachyonRuntimeSources
    runtime_facade.cpp
    
    compiler/scene_compiler.cpp
    compiler/layer_compiler.cpp
    
    execution/render_session.cpp
    execution/render_session_parallel.cpp
    execution/render_session_audio.cpp
    execution/frame_executor.cpp
    execution/render_plan.cpp
    execution/render_plan_build.cpp
    execution/native_render.cpp
    execution/compiled_frame_program.cpp
    
    policy/worker_policy.cpp
    policy/surface_pool_policy.cpp
    
    telemetry/process_resource_sampler.cpp
    
    registry/engine_registry.cpp
    
    resource/render_context.cpp
    resource/surface_pool.cpp
    resource/surface_pool_fast.cpp
    
    media/media_pipeline.cpp
    media/probe_cache.cpp
    
    profiling/render_profiler.cpp
    diagnostics/report.cpp
)

add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
target_link_libraries(TachyonRuntime
    PUBLIC
        TachyonScene
        TachyonRenderer2D
        TachyonBackends
        TachyonAssetOps
    PRIVATE
        TachyonPlatform
)
