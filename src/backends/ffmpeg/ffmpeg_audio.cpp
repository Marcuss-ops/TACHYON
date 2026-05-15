#include "tachyon/backends/ffmpeg/ffmpeg_audio_extractor.h"
#include "tachyon/core/platform/process.h"
#include <vector>
#include <string>

namespace tachyon::backends::ffmpeg {

using namespace core::media;
using core::MediaResult;
using core::MediaError;
using core::MediaErrorCode;

MediaResult<void> FFmpegAudioExtractor::extract(const AudioExtractConfig& config) {
    std::vector<std::string> args = {
        "-y",
        "-i", config.input_file.string(),
        "-vn",
        "-acodec", "pcm_s16le",
        "-ar", std::to_string(config.sample_rate),
        "-ac", std::to_string(config.channels),
        config.output_wav.string()
    };

    ::tachyon::core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    
    auto proc_res = ::tachyon::core::platform::run_process(spec);
    if (!proc_res.success || proc_res.exit_code != 0) {
        return MediaResult<void>::failure(MediaError(MediaErrorCode::Audio, "FFmpeg audio extraction failed: " + proc_res.error));
    }
    
    return MediaResult<void>::success();
}

} // namespace tachyon::backends::ffmpeg
