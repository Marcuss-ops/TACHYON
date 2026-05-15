# Tachyon FFmpeg Backend Module

set(TachyonBackendFfmpegSources
    ffmpeg_probe.cpp
    ffmpeg_clip_processor.cpp
    ffmpeg_overlay_merger.cpp
    ffmpeg_transition_filters.cpp
    ffmpeg_audio.cpp
    ffmpeg_video_concat.cpp
    ffmpeg_encoder.cpp
    register_ffmpeg_backend.cpp
)

add_library(TachyonBackendFfmpeg STATIC ${TachyonBackendFfmpegSources})
tachyon_configure_common(TachyonBackendFfmpeg)

target_include_directories(TachyonBackendFfmpeg 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(TachyonBackendFfmpeg
    PUBLIC
        TachyonCore
        TachyonPlatform
)

# Link FFmpeg libraries
tachyon_link_media_deps(TachyonBackendFfmpeg)

set_target_properties(TachyonBackendFfmpeg PROPERTIES POSITION_INDEPENDENT_CODE ON)
