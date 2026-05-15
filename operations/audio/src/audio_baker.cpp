#include "tachyon/audio/audio_baker.h"
#include "tachyon/audio/gate_utils.h"
#include "tachyon/audio/audio_filter_builder.h"
#include "tachyon/core/platform/process.h"
#include <sstream>
#include <iomanip>

namespace tachyon::audio {

core::MediaResult<void> AudioBaker::bake_master_audio(const AudioBakeConfig& config) {
    if (config.base_video_path.empty()) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::IO, "Base video path is empty")
        );
    }

    auto cmd_args = build_bake_command(config);
    
    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg"; // Assume ffmpeg is in PATH
    spec.args = cmd_args;
    spec.timeout = std::chrono::minutes(5); // Reasonable timeout
    
    auto result = core::platform::run_process(spec);
    
    if (!result.success || result.exit_code != 0) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::Audio, "FFmpeg audio baking failed: " + result.error)
                .with_stage("bake_master_audio")
        );
    }
    
    if (!std::filesystem::exists(config.output_path)) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::IO, "Audio baking succeeded but output file is missing")
                .with_path(config.output_path.string())
        );
    }
    
    return core::MediaResult<void>::success();
}

std::string AudioBaker::build_bake_filter(const AudioBakeConfig& config) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    int input_idx = 0;
    
    // 1. Base video audio [0:a]
    std::string gate_expr = GateUtils::build_gate_expr_from_ranges(config.gate_ranges, false);
    ss << "[0:a]volume=eval=frame:volume='" << gate_expr << "'[base_gated];";
    input_idx++;

    // 2. Voiceover [1:a] if present
    if (config.voiceover_path) {
        ss << "[" << input_idx << ":a]";
        if (config.voiceover_offset_seconds > 0) {
            long ms = static_cast<long>(config.voiceover_offset_seconds * 1000.0);
            ss << "adelay=" << ms << "|" << ms << "[vo_delayed];";
        } else {
            ss << "anull[vo_delayed];";
        }
        input_idx++;
    }

    // 3. Music [2:a] if present
    if (config.background_music_path) {
        ss << "[" << input_idx << ":a]volume=" << config.music_volume_db << "dB[music_attenuated];";
        input_idx++;
    }

    // 4. Mix everything
    ss << "[base_gated]";
    if (config.voiceover_path) ss << "[vo_delayed]";
    if (config.background_music_path) ss << "[music_attenuated]";
    
    int mix_inputs = 1 + (config.voiceover_path ? 1 : 0) + (config.background_music_path ? 1 : 0);
    ss << "amix=inputs=" << mix_inputs << ":duration=longest[out_mixed];";
    
    // 5. Final processing (limiter)
    ss << "[out_mixed]alimiter=level_in=1:level_out=0.9:limit=0.9:attack=5:release=50:asc=1:asc_level=0.5[out]";
    
    return ss.str();
}

std::vector<std::string> AudioBaker::build_bake_command(const AudioBakeConfig& config) {
    std::vector<std::string> args;
    args.push_back("-y"); // Overwrite
    
    // Inputs
    args.push_back("-i");
    args.push_back(config.base_video_path.string());
    
    if (config.voiceover_path) {
        args.push_back("-i");
        args.push_back(config.voiceover_path->string());
    }
    
    if (config.background_music_path) {
        args.push_back("-stream_loop");
        args.push_back("-1"); // Infinite loop for music
        args.push_back("-i");
        args.push_back(config.background_music_path->string());
    }
    
    // Filter
    args.push_back("-filter_complex");
    args.push_back(build_bake_filter(config));
    
    // Map
    args.push_back("-map");
    args.push_back("[out]");
    
    // Output codec
    if (config.use_aac) {
        args.push_back("-c:a");
        args.push_back("aac");
        args.push_back("-b:a");
        args.push_back("256k");
    } else {
        args.push_back("-c:a");
        args.push_back("pcm_s16le");
    }
    
    args.push_back("-ar");
    args.push_back(std::to_string(config.sample_rate));
    
    args.push_back(config.output_path.string());
    
    return args;
}

} // namespace tachyon::audio
