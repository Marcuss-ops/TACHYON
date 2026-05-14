#include "tachyon/media/clip_processor.h"
#include "tachyon/media/probe.h"
#include "tachyon/core/platform/process.h"
#include <sstream>
#include <iomanip>

namespace tachyon::media {

core::MediaResult<void> ClipProcessor::process_clip(const ClipProcessingConfig& config) {
    if (config.input_path.empty()) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::IO, "Input path is empty")
        );
    }

    // Probe duration
    auto probe_result = MediaProbe::probe_file(config.input_path);
    if (!probe_result.ok()) {
        return core::MediaResult<void>::failure(probe_result.error.value());
    }
    double duration = probe_result.value->duration_seconds;

    std::vector<std::string> args;
    args.push_back("-y");
    
    // Main input
    args.push_back("-i");
    args.push_back(config.input_path.string());
    
    // SFX inputs
    for (const auto& sfx : config.sfx) {
        args.push_back("-i");
        args.push_back(sfx.path.string());
    }
    
    args.push_back("-filter_complex");
    args.push_back(build_filter_complex(config, duration));
    
    args.push_back("-map");
    args.push_back("[out_v]");
    args.push_back("-map");
    args.push_back("[out_a]");
    
    args.push_back("-c:v");
    args.push_back("libx264");
    args.push_back("-crf");
    args.push_back(std::to_string(config.crf));
    args.push_back("-preset");
    args.push_back("fast");
    args.push_back("-r");
    args.push_back(std::to_string(config.fps));
    
    args.push_back("-c:a");
    args.push_back("aac");
    args.push_back("-b:a");
    args.push_back("256k");
    
    args.push_back(config.output_path.string());

    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    spec.timeout = std::chrono::minutes(10);
    
    auto result = core::platform::run_process(spec);
    
    if (!result.success || result.exit_code != 0) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::Encode, "FFmpeg clip processing failed: " + result.error)
                .with_stage("process_clip")
        );
    }

    return core::MediaResult<void>::success();
}

std::string ClipProcessor::build_filter_complex(const ClipProcessingConfig& config, double duration) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    // 1. Video scaling and padding to match target resolution
    ss << "[0:v]scale=" << config.width << ":" << config.height << ":force_original_aspect_ratio=decrease,";
    ss << "pad=" << config.width << ":" << config.height << ":(ow-iw)/2:(oh-ih)/2,setsar=1";
    
    // 2. Video fades
    if (config.fade_in_seconds > 0) {
        ss << ",fade=t=in:st=0:d=" << config.fade_in_seconds;
    }
    if (config.fade_out_seconds > 0) {
        ss << ",fade=t=out:st=" << (duration - config.fade_out_seconds) << ":d=" << config.fade_out_seconds;
    }
    
    // 3. Subtitles
    if (config.subtitles_srt) {
        ss << ",subtitles='" << config.subtitles_srt->string() << "'";
        if (config.font_path) {
            ss << ":fontsdir='" << config.font_path->parent_path().string() << "'";
        }
    }
    
    ss << "[out_v];";
    
    // 4. Audio processing (original + SFX)
    ss << "[0:a]volume=1.0[main_a];";
    
    int sfx_count = static_cast<int>(config.sfx.size());
    for (int i = 0; i < sfx_count; ++i) {
        int input_idx = i + 1;
        long ms = static_cast<long>(config.sfx[i].start_time * 1000.0);
        ss << "[" << input_idx << ":a]volume=" << config.sfx[i].volume << ",adelay=" << ms << "|" << ms << "[sfx" << i << "];";
    }
    
    // Mix audio
    ss << "[main_a]";
    for (int i = 0; i < sfx_count; ++i) {
        ss << "[sfx" << i << "]";
    }
    ss << "amix=inputs=" << (sfx_count + 1) << ":duration=longest[out_a]";
    
    return ss.str();
}

} // namespace tachyon::media
