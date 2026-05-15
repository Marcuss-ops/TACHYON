#include "tachyon/backends/backend_registry.h"
#include "tachyon/backends/ffmpeg/ffmpeg_probe.h"
#include "tachyon/backends/ffmpeg/ffmpeg_encoder.h"
#include "tachyon/backends/ffmpeg/ffmpeg_clip_processor.h"
#include "tachyon/backends/ffmpeg/ffmpeg_overlay_merger.h"
#include "tachyon/backends/ffmpeg/ffmpeg_audio_extractor.h"
#include "tachyon/backends/ffmpeg/ffmpeg_video_concat.h"

namespace tachyon::backends::ffmpeg {

void register_backend() {
    auto& reg = BackendRegistry::instance();

    reg.register_probe("ffmpeg", []() {
        return std::make_unique<FFmpegProbe>();
    });

    reg.register_video_encoder("ffmpeg", []() {
        return std::make_unique<FFmpegEncoder>();
    });

    reg.register_clip_processor("ffmpeg", []() {
        return std::make_unique<FFmpegClipProcessor>();
    });

    reg.register_overlay_merger("ffmpeg", []() {
        return std::make_unique<FFmpegOverlayMerger>();
    });

    reg.register_audio_extractor("ffmpeg", []() {
        return std::make_unique<FFmpegAudioExtractor>();
    });

    reg.register_video_concat("ffmpeg", []() {
        return std::make_unique<FFmpegVideoConcat>();
    });
}

} // namespace tachyon::backends::ffmpeg
