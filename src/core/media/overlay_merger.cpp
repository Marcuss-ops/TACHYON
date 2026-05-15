#include "tachyon/core/media/overlay_merger.h"
#include "tachyon/core/platform/process.h"
#include <sstream>
#include <iomanip>

namespace tachyon::core::media {

MediaResult<void> OverlayMerger::merge_overlays(const OverlayMergeConfig& config) {
    if (config.base_video_path.empty()) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::IO, "Base video path is empty")
        );
    }

    std::vector<std::string> args;
    args.push_back("-y");
    
    // Base video
    args.push_back("-i");
    args.push_back(config.base_video_path.string());
    
    // Overlay inputs
    for (const auto& overlay : config.overlays) {
        args.push_back("-i");
        args.push_back(overlay.path.string());
    }
    
    args.push_back("-filter_complex");
    args.push_back(build_filter_complex(config));
    
    args.push_back("-map");
    args.push_back("[out]");
    args.push_back("-map");
    args.push_back("0:a?"); // Optional audio from base video
    
    args.push_back("-c:v");
    args.push_back("libx264");
    args.push_back("-crf");
    args.push_back(std::to_string(config.crf));
    args.push_back("-preset");
    args.push_back("fast");
    args.push_back("-c:a");
    args.push_back("aac");
    args.push_back("-b:a");
    args.push_back("256k");
    
    args.push_back("-avoid_negative_ts");
    args.push_back("make_zero");
    
    args.push_back(config.output_path.string());

    platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    spec.timeout = std::chrono::minutes(15);
    
    auto result = platform::run_process(spec);
    
    if (!result.success || result.exit_code != 0) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::Encode, "FFmpeg overlay merge failed: " + result.error)
                .with_stage("merge_overlays")
        );
    }

    return MediaResult<void>::success();
}

std::string OverlayMerger::build_filter_complex(const OverlayMergeConfig& config) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    std::string last_output = "0:v";
    
    for (size_t i = 0; i < config.overlays.size(); ++i) {
        const auto& overlay = config.overlays[i];
        int input_idx = static_cast<int>(i) + 1;
        std::string current_output = "v" + std::to_string(i);
        
        // Offset the overlay in time
        ss << "[" << input_idx << ":v]setpts=PTS-STARTPTS+" << overlay.start_time << "/TB[ovl" << i << "];";
        
        // Apply the overlay
        ss << "[" << last_output << "][ovl" << i << "]overlay=";
        ss << "x=" << overlay.x << ":y=" << overlay.y << ":";
        ss << "enable='" << build_enable_condition(overlay, config.middle_clip_windows) << "'";
        
        ss << "[" << current_output << "];";
        last_output = current_output;
    }
    
    // Final output with subtitles if present
    if (config.subtitles_ass_path) {
        ss << "[" << last_output << "]ass='" << config.subtitles_ass_path->string() << "'";
        if (config.fonts_dir) {
            ss << ":fontsdir='" << config.fonts_dir->string() << "'";
        }
        ss << "[out]";
    } else {
        // Just rename the last output to [out]
        ss << "[" << last_output << "]null[out]";
    }
    
    return ss.str();
}

std::string OverlayMerger::build_enable_condition(
    const OverlaySpec& overlay, 
    const std::vector<std::pair<double, double>>& middle_clips) 
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    double end_time = overlay.start_time + overlay.duration;
    ss << "between(t," << overlay.start_time << "," << end_time << ")";
    
    for (const auto& window : middle_clips) {
        ss << "*not(between(t," << window.first << "," << window.second << "))";
    }
    
    return ss.str();
}

} // namespace tachyon::core::media
