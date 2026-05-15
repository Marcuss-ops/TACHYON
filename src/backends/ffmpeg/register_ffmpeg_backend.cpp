#include "tachyon/backends/backend_registry.h"
#include "tachyon/backends/ffmpeg/ffmpeg_probe.h"
#include "tachyon/backends/ffmpeg/ffmpeg_encoder.h"

namespace tachyon::backends::ffmpeg {

void register_backend() {
    auto& reg = BackendRegistry::instance();

    reg.register_probe("ffmpeg", []() {
        return std::make_unique<FFmpegProbe>();
    });

    reg.register_video_encoder("ffmpeg", []() {
        return std::make_unique<FFmpegEncoder>();
    });
}

} // namespace tachyon::backends::ffmpeg
