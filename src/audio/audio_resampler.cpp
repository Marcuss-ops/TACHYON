#include "tachyon/audio/audio_resampler.h"
#include <sstream>
#include <iostream>

namespace tachyon::audio {

tachyon::core::MediaResult<std::string> AudioResampler::resample(const Config& config) {
    if (!std::filesystem::exists(config.input_path)) {
        return tachyon::core::MediaResult<std::string>::failure(
            tachyon::core::MediaError(tachyon::core::MediaErrorCode::Init, "Input file does not exist: " + config.input_path.string())
                .with_stage("audio")
                .with_path(config.input_path.string())
        );
    }

    std::string cmd = build_ffmpeg_command(config);
    int ret = std::system(cmd.c_str());

    if (ret != 0) {
        return tachyon::core::MediaResult<std::string>::failure(
            tachyon::core::MediaError(tachyon::core::MediaErrorCode::Pipeline, "FFmpeg resampling failed with code " + std::to_string(ret))
                .with_stage("audio")
                .with_path(config.output_path.string())
        );
    }

    if (!std::filesystem::exists(config.output_path)) {
        return tachyon::core::MediaResult<std::string>::failure(
            tachyon::core::MediaError(tachyon::core::MediaErrorCode::Pipeline, "FFmpeg finished but output file was not created")
                .with_stage("audio")
                .with_path(config.output_path.string())
        );
    }

    return tachyon::core::MediaResult<std::string>::success(config.output_path.string());
}

std::string AudioResampler::build_ffmpeg_command(const Config& config) {
    std::stringstream ss;
    ss << "ffmpeg -hide_banner -loglevel error -y -i \"" << config.input_path.string() << "\" ";
    
    // Resampling parameters
    ss << "-ar " << config.sample_rate << " -ac " << config.channels << " ";
    
    if (config.format == "aac") {
        ss << "-c:a aac -b:a " << config.bitrate_kbps << "k ";
    } else {
        ss << "-c:a pcm_s16le ";
    }
    
    ss << "\"" << config.output_path.string() << "\"";
    return ss.str();
}

} // namespace tachyon::audio
