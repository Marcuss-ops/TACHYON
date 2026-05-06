add_library(TachyonSceneEval STATIC ${TachyonSceneEvalSources})
tachyon_configure_common(TachyonSceneEval)
target_link_libraries(TachyonSceneEval
    PUBLIC
        TachyonTimeline
    PRIVATE
        TachyonCore
        TachyonColor
        TachyonText
        TachyonAudio
        TachyonMedia
        TachyonRenderer2D
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonRenderer3D>
        $<$<BOOL:${TACHYON_ENABLE_3D}>:TachyonScene3DBridge>
)
tachyon_link_text_deps(TachyonSceneEval)
tachyon_link_media_deps(TachyonSceneEval)
tachyon_link_3d_deps(TachyonSceneEval)
tachyon_link_omp(TachyonSceneEval)

add_library(TachyonPresets STATIC ${TachyonPresetsSources})
tachyon_configure_common(TachyonPresets)
target_link_libraries(TachyonPresets
    PUBLIC
        TachyonCore
        TachyonText
        TachyonAudio
        TachyonMedia
)
tachyon_link_text_deps(TachyonPresets)
tachyon_link_media_deps(TachyonPresets)
tachyon_link_omp(TachyonPresets)

add_library(TachyonScene SHARED ${TachyonSceneSources})
tachyon_configure_common(TachyonScene)
target_compile_definitions(TachyonScene 
    PRIVATE TACHYON_BUILD_DLL
    INTERFACE TACHYON_USE_DLL
)
target_link_libraries(TachyonScene
    PUBLIC
        TachyonSceneEval
        TachyonTimeline
    PRIVATE
        TachyonCore
        TachyonColor
        TachyonPlatform
        TachyonPresets
        TachyonRenderer2D
        TachyonAudio
        TachyonMedia
)
if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonScene PRIVATE TachyonTracker)
endif()
tachyon_link_omp(TachyonScene)
