#pragma once

#include "tachyon/runtime/media/media_services.h"
#include "tachyon/backends/ffmpeg/ffmpeg_probe.h"
#include "tachyon/backends/ffmpeg/ffmpeg_clip_processor.h"
#include "tachyon/backends/ffmpeg/ffmpeg_overlay_merger.h"
#include "tachyon/backends/ffmpeg/ffmpeg_audio_extractor.h"
#include "tachyon/backends/ffmpeg/ffmpeg_video_concat.h"
#include "tachyon/backends/whisper/whisper_audio_analyzer.h"

namespace tachyon::backends {

/**
 * @brief Concrete implementation bundle using FFmpeg and Whisper.
 */
struct MediaBackendBundle {
    ffmpeg::FFmpegProbe probe;
    ffmpeg::FFmpegClipProcessor clip_processor;
    ffmpeg::FFmpegOverlayMerger overlay_merger;
    ffmpeg::FFmpegAudioExtractor audio_extractor;
    ffmpeg::FFmpegVideoConcat video_concat;
    whisper::WhisperAudioAnalyzer audio_analyzer;

    /**
     * @brief Provides a view of the backends as runtime media services.
     */
    runtime::media::MediaServices services() {
        return {
            probe,
            clip_processor,
            overlay_merger,
            audio_extractor,
            audio_analyzer,
            video_concat
        };
    }
};

} // namespace tachyon::backends
