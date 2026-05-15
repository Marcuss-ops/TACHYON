#include "tachyon/backends/ffmpeg/ffmpeg_clip_processor.h"
#include "tachyon/backends/ffmpeg/ffmpeg_probe.h"
#include "tachyon/core/platform/process.h"
#include <sstream>
#include <iomanip>

namespace tachyon::backends::ffmpeg {

using namespace core::media;
using core::MediaResult;
using core::MediaError;
using core::MediaErrorCode;

MediaResult<void> FFmpegClipProcessor::process_clip(const ClipProcessingConfig& config) {
    // 1. Probe for duration
    FFmpegProbe probe;
    auto meta_res = probe.probe_file(config.input_path);
    if (!meta_res.ok()) return MediaResult<void>::failure(*meta_res.error);
    
    double duration = meta_res.value->duration_seconds;

    std::vector<std::string> args;
    args.push_back("-y");
    args.push_back("-i");
    args.push_back(config.input_path.string());
    
    // Add SFX inputs
    for (const auto& sfx : config.sfx) {
        args.push_back("-i");
        args.push_back(sfx.path.string());
    }

    args.push_back("-filter_complex");
    args.push_back(build_filter_complex(config, duration));
    
    args.push_back("-map");
    args.push_back("[v_out]");
    args.push_back("-map");
    args.push_back("[a_out]");
    
    args.push_back("-c:v");
    args.push_back("libx264");
    args.push_back("-crf");
    args.push_back(std::to_string(config.crf));
    args.push_back("-preset");
    args.push_back("fast");
    
    args.push_back("-c:a");
    args.push_back("aac");
    args.push_back("-b:a");
    args.push_back("192k");
    
    args.push_back(config.output_path.string());

    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    
    auto result = core::platform::run_process(spec);
    if (!result.success || result.exit_code != 0) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::Decode, "FFmpeg clip processing failed: " + result.error)
                .with_stage("clip_process")
        );
    }

    return MediaResult<void>::success();
}

std::string FFmpegClipProcessor::build_filter_complex(const ClipProcessingConfig& config, double duration) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    // Video: scaling and fades
    ss << "[0:v]scale=" << config.width << ":" << config.height << ":force_original_aspect_ratio=decrease,pad=" << config.width << ":" << config.height << ":(ow-iw)/2:(oh-ih)/2";
    
    if (config.fade_in_seconds > 0) {
        ss << ",fade=t=in:st=0:d=" << config.fade_in_seconds;
    }
    if (config.fade_out_seconds > 0) {
        ss << ",fade=t=out:st=" << (duration - config.fade_out_seconds) << ":d=" << config.fade_out_seconds;
    }
    
    if (config.subtitles_srt) {
        ss << ",subtitles='" << config.subtitles_srt->string() << "'";
        if (config.font_path) {
            ss << ":fontsdir='" << config.font_path->parent_path().string() << "'";
        }
    }
    
    ss << "[v_out];";
    
    // Audio: mixdown and SFX
    std::string last_audio = "0:a";
    for (size_t i = 0; i < config.sfx.size(); ++i) {
        const auto& sfx = config.sfx[i];
        int sfx_idx = static_cast<int>(i) + 1;
        std::string current_audio = "amix" + std::to_string(i);
        
        // Delay SFX
        ss << "[" << sfx_idx << ":a]adelay=" << static_cast<int>(sfx.start_time * 1000) << "|" << static_cast<int>(sfx.start_time * 1000) << "[sfx" << i << "];";
        
        // Mix
        ss << "[" << last_audio << "][sfx" << i << "]amix=inputs=2:duration=first[ " << current_audio << "];";
        last_audio = current_audio;
    }
    
    ss << "[" << last_audio << "]volume=1.0[a_out]";
    
    return ss.str();
}

} // namespace tachyon::backends::ffmpeg
