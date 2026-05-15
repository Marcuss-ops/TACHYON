# Tachyon Runtime Module

set(TachyonRuntimeSources
    runtime_facade.cpp
    compiler/scene_compiler.cpp
    execution/render_session.cpp
    execution/frame_executor.cpp
    registry/engine_registry.cpp
    resource/render_context.cpp
    resource/surface_pool.cpp
    resource/surface_pool_fast.cpp
    policy/surface_pool_policy.cpp
    media/media_pipeline.cpp
    media/probe_cache.cpp
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
