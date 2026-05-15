# Tachyon Backends Module

set(TachyonBackendSources
    ffmpeg/ffmpeg_probe.cpp
    ffmpeg/ffmpeg_clip_processor.cpp
    ffmpeg/ffmpeg_overlay_merger.cpp
    ffmpeg/ffmpeg_transition_filters.cpp
    ffmpeg/ffmpeg_audio.cpp
    ffmpeg/ffmpeg_video_concat.cpp
    whisper/whisper_audio_analyzer.cpp
)

add_library(TachyonBackends STATIC ${TachyonBackendSources})
tachyon_configure_common(TachyonBackends)

target_link_libraries(TachyonBackends
    PUBLIC
        TachyonCore
        TachyonPlatform
)

# Media/FFmpeg integration
tachyon_link_media_deps(TachyonBackends)
