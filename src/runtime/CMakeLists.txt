# Tachyon Runtime Module

set(TachyonRuntimeSources
    runtime_facade.cpp
    compiler/scene_compiler.cpp
    execution/render_session.cpp
    execution/frame_executor.cpp
    registry/runtime_registry_bundle.cpp
)

add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
target_link_libraries(TachyonRuntime
    PUBLIC
        TachyonScene
        TachyonRenderer2D
        TachyonOperations
    PRIVATE
        TachyonPlatform
)

# Compatibility aliases
tachyon_deprecate_target(TachyonRuntimeCore TachyonRuntime)
tachyon_deprecate_target(TachyonRuntimeExecution TachyonRuntime)
tachyon_deprecate_target(TachyonRuntimeCompiler TachyonRuntime)
tachyon_deprecate_target(TachyonRuntimeTelemetry TachyonRuntime)
tachyon_deprecate_target(TachyonRuntimePlayback TachyonRuntime)
tachyon_deprecate_target(TachyonRuntimeEngine TachyonRuntime)
