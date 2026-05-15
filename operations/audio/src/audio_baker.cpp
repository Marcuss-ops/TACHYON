#include "tachyon/audio/audio_baker.h"
#include "tachyon/core/audio/audio_filter_utils.h"
#include "tachyon/core/platform/process.h"

namespace tachyon::audio {

core::MediaResult<void> AudioBaker::bake_master_audio(const AudioBakeConfig& config) {
    if (config.base_video_path.empty()) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::IO, "Base video path is empty")
        );
    }

    std::string vo_path_str;
    const std::string* vo_ptr = nullptr;
    if (config.voiceover_path) {
        vo_path_str = config.voiceover_path->string();
        vo_ptr = &vo_path_str;
    }

    std::string bg_path_str;
    const std::string* bg_ptr = nullptr;
    if (config.background_music_path) {
        bg_path_str = config.background_music_path->string();
        bg_ptr = &bg_path_str;
    }

    auto cmd_args = core::audio::AudioFilterUtils::build_bake_command(
        config.base_video_path.string(),
        vo_ptr,
        bg_ptr,
        config.output_path.string(),
        config.gate_ranges,
        config.voiceover_offset_seconds,
        config.music_volume_db,
        config.sample_rate,
        config.use_aac
    );
    
    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg"; 
    spec.args = cmd_args;
    spec.timeout = std::chrono::minutes(5);
    
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

} // namespace tachyon::audio
