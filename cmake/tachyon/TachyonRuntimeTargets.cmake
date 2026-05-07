# 1. TachyonRuntimeCore: The base foundation
add_library(TachyonRuntimeCore STATIC ${TachyonRuntimeCoreSources})
tachyon_configure_common(TachyonRuntimeCore)
target_link_libraries(TachyonRuntimeCore
    PUBLIC
        TachyonCore
        TachyonColor
)

# 2. TachyonRuntimeExecution: The rendering pipeline and engine logic
add_library(TachyonRuntimeExecution STATIC ${TachyonRuntimeExecutionSources})
tachyon_configure_common(TachyonRuntimeExecution)
if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonRuntimeExecution PRIVATE TACHYON_ENABLE_AUDIO_MUX)
endif()
target_link_libraries(TachyonRuntimeExecution
    PUBLIC
        TachyonRuntimeCore
        TachyonScene
        TachyonRenderer2D
        TachyonMedia
        TachyonOutput
    PRIVATE
        TachyonPlatform
        TachyonAudio
        TachyonText
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonRenderer3D>
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonScene3DBridge>
)
tachyon_link_3d_deps(TachyonRuntimeExecution)
tachyon_link_text_deps(TachyonRuntimeExecution)
tachyon_link_media_deps(TachyonRuntimeExecution)
tachyon_link_output_deps(TachyonRuntimeExecution)
tachyon_link_omp(TachyonRuntimeExecution)

# 3. TachyonRuntimeCompiler: Scene compilation logic
add_library(TachyonRuntimeCompiler STATIC ${TachyonRuntimeCompilerSources})
tachyon_configure_common(TachyonRuntimeCompiler)
target_link_libraries(TachyonRuntimeCompiler
    PUBLIC
        TachyonRuntimeCore
        TachyonScene
)

# 4. TachyonRuntimeTelemetry: Profiling and diagnostics
add_library(TachyonRuntimeTelemetry STATIC 
    ${TachyonRuntimeProfilingSources}
    ${TachyonRuntimeTelemetrySources}
)
tachyon_configure_common(TachyonRuntimeTelemetry)
target_link_libraries(TachyonRuntimeTelemetry
    PUBLIC
        TachyonRuntimeCore
)

# 5. TachyonRuntimePlayback: Playback engine and queue
add_library(TachyonRuntimePlayback STATIC ${TachyonRuntimePlaybackSources})
tachyon_configure_common(TachyonRuntimePlayback)
target_link_libraries(TachyonRuntimePlayback
    PUBLIC
        TachyonRuntimeCore
        TachyonAudio
)

# 6. TachyonRuntime: The aggregate interface
add_library(TachyonRuntime INTERFACE)
target_link_libraries(TachyonRuntime INTERFACE
    TachyonRuntimeCore
    TachyonRuntimeExecution
    TachyonRuntimeCompiler
    TachyonRuntimeTelemetry
    TachyonRuntimePlayback
)

# Legacy compatibility target
add_library(TachyonRuntimeEngine INTERFACE)
target_link_libraries(TachyonRuntimeEngine INTERFACE
    TachyonRuntime
)
