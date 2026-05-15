#include "tachyon/core/audio/audio_filter_utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace tachyon::core::audio {

std::string AudioFilterUtils::build_gate_expr_from_ranges(
    const std::vector<std::pair<double, double>>& ranges,
    bool inverted) 
{
    if (ranges.empty()) {
        return inverted ? "0" : "1";
    }

    std::vector<std::pair<double, double>> valid_ranges;
    for (const auto& range : ranges) {
        if (range.first < range.second && range.first >= 0) {
            valid_ranges.push_back(range);
        }
    }

    if (valid_ranges.empty()) {
        return inverted ? "0" : "1";
    }

    std::sort(valid_ranges.begin(), valid_ranges.end());

    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "if(";
    for (size_t i = 0; i < valid_ranges.size(); ++i) {
        if (i > 0) ss << "+";
        ss << "between(t," << valid_ranges[i].first << "," << valid_ranges[i].second << ")";
    }
    
    if (inverted) {
        ss << ",1,0)";
    } else {
        ss << ",0,1)";
    }

    return ss.str();
}

std::string AudioFilterUtils::build_intro_only_gate_expr(double duration) {
    if (duration <= 0) return "0";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "if(lt(t," << duration << "),1,0)";
    return ss.str();
}

std::string AudioFilterUtils::build_amix_filter(const AmixConfig& config) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    if (config.offset_seconds > 0) {
        ss << "atrim=start=" << config.offset_seconds << ",asetpts=PTS-STARTPTS,";
    }
    
    if (config.volume_db != 0.0) {
        ss << "volume=" << config.volume_db << "dB,";
    }
    
    ss << "amix=inputs=" << config.inputs << ":duration=longest";
    return ss.str();
}

std::string AudioFilterUtils::build_background_music_filter(
    double duration,
    double fade_in_seconds,
    double fade_out_seconds,
    double volume_db) 
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    ss << "atrim=duration=" << duration << ",";
    
    if (fade_in_seconds > 0) {
        ss << "afade=t=in:st=0:d=" << fade_in_seconds << ",";
    }
    
    if (fade_out_seconds > 0) {
        ss << "afade=t=out:st=" << (duration - fade_out_seconds) << ":d=" << fade_out_seconds << ",";
    }
    
    if (volume_db != 0.0) {
        ss << "volume=" << volume_db << "dB";
    } else {
        std::string s = ss.str();
        if (!s.empty() && s.back() == ',') s.pop_back();
        return s;
    }
    
    return ss.str();
}

std::string AudioFilterUtils::build_audio_delay_filter(double delay_seconds) {
    std::stringstream ss;
    long ms = static_cast<long>(delay_seconds * 1000.0);
    ss << "adelay=" << ms << "|" << ms;
    return ss.str();
}

std::string AudioFilterUtils::build_acrossfade_filter(double duration_seconds) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "acrossfade=d=" << duration_seconds << ":c1=tri:c2=tri";
    return ss.str();
}

std::string AudioFilterUtils::build_concat_audio_filter(int input_count) {
    std::stringstream ss;
    for (int i = 0; i < input_count; ++i) {
        ss << "[" << i << ":a]";
    }
    ss << "concat=n=" << input_count << ":v=0:a=1";
    return ss.str();
}

std::string AudioFilterUtils::build_bake_filter(
    const std::vector<std::pair<double, double>>& gate_ranges,
    bool has_voiceover,
    double voiceover_offset_seconds,
    bool has_background_music,
    double music_volume_db) 
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    int input_idx = 0;
    
    // 1. Base video audio [0:a]
    std::string gate_expr = build_gate_expr_from_ranges(gate_ranges, false);
    ss << "[0:a]volume=eval=frame:volume='" << gate_expr << "'[base_gated];";
    input_idx++;

    // 2. Voiceover [1:a] if present
    if (has_voiceover) {
        ss << "[" << input_idx << ":a]";
        if (voiceover_offset_seconds > 0) {
            long ms = static_cast<long>(voiceover_offset_seconds * 1000.0);
            ss << "adelay=" << ms << "|" << ms << "[vo_delayed];";
        } else {
            ss << "anull[vo_delayed];";
        }
        input_idx++;
    }

    // 3. Music [2:a] if present
    if (has_background_music) {
        ss << "[" << input_idx << ":a]volume=" << music_volume_db << "dB[music_attenuated];";
        input_idx++;
    }

    // 4. Mix everything
    ss << "[base_gated]";
    if (has_voiceover) ss << "[vo_delayed]";
    if (has_background_music) ss << "[music_attenuated]";
    
    int mix_inputs = 1 + (has_voiceover ? 1 : 0) + (has_background_music ? 1 : 0);
    ss << "amix=inputs=" << mix_inputs << ":duration=longest[out_mixed];";
    
    // 5. Final processing (limiter)
    ss << "[out_mixed]alimiter=level_in=1:level_out=0.9:limit=0.9:attack=5:release=50:asc=1:asc_level=0.5[out]";
    
    return ss.str();
}

std::vector<std::string> AudioFilterUtils::build_bake_command(
    const std::string& base_video_path,
    const std::string* voiceover_path,
    const std::string* background_music_path,
    const std::string& output_path,
    const std::vector<std::pair<double, double>>& gate_ranges,
    double voiceover_offset_seconds,
    double music_volume_db,
    int sample_rate,
    bool use_aac) 
{
    std::vector<std::string> args;
    args.push_back("-y"); // Overwrite
    
    // Inputs
    args.push_back("-i");
    args.push_back(base_video_path);
    
    if (voiceover_path) {
        args.push_back("-i");
        args.push_back(*voiceover_path);
    }
    
    if (background_music_path) {
        args.push_back("-stream_loop");
        args.push_back("-1"); // Infinite loop for music
        args.push_back("-i");
        args.push_back(*background_music_path);
    }
    
    // Filter
    args.push_back("-filter_complex");
    args.push_back(build_bake_filter(
        gate_ranges, 
        voiceover_path != nullptr, 
        voiceover_offset_seconds, 
        background_music_path != nullptr, 
        music_volume_db));
    
    // Map
    args.push_back("-map");
    args.push_back("[out]");
    
    // Output codec
    if (use_aac) {
        args.push_back("-c:a");
        args.push_back("aac");
        args.push_back("-b:a");
        args.push_back("256k");
    } else {
        args.push_back("-c:a");
        args.push_back("pcm_s16le");
    }
    
    args.push_back("-ar");
    args.push_back(std::to_string(sample_rate));
    
    args.push_back(output_path);
    
    return args;
}

} // namespace tachyon::core::audio
