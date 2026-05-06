add_library(TachyonRuntime STATIC ${TachyonRuntimeSources})
tachyon_configure_common(TachyonRuntime)
if(TACHYON_ENABLE_AUDIO_MUX)
    target_compile_definitions(TachyonRuntime PRIVATE TACHYON_ENABLE_AUDIO_MUX)
endif()
target_link_libraries(TachyonRuntime
    PUBLIC
        TachyonScene
        TachyonCore
        TachyonColor
        TachyonText
        TachyonAudio
        TachyonMedia
        TachyonOutput
        TachyonPlatform
        TachyonRenderer2D
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonRenderer3D>
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonScene3DBridge>
)
tachyon_link_3d_deps(TachyonRuntime)
tachyon_link_text_deps(TachyonRuntime)
tachyon_link_media_deps(TachyonRuntime)
tachyon_link_output_deps(TachyonRuntime)
tachyon_link_omp(TachyonRuntime)
if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonRuntime PRIVATE TachyonTracker)
endif()

add_library(TachyonRuntimeEngine INTERFACE)
target_link_libraries(TachyonRuntimeEngine INTERFACE
    TachyonRuntime
    TachyonScene
    TachyonRenderer2D
    TachyonText
    TachyonMedia
    $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonRenderer3D>
    TachyonCore
    TachyonAudio
    TachyonOutput
    TachyonPlatform
)
