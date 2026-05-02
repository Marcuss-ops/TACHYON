#include <filesystem>
#include <string>
#include <cstdlib>
#include <system_error>

#include "tachyon/core/platform/process.h"
#include "tachyon/output/output_utils.h"
#include "tachyon/runtime/execution/session/render_internal.h"

namespace tachyon {

bool mux_audio_video(const RenderPlan& plan, const std::string& video_path, const std::string& audio_path, std::string& error) {
    if (!std::filesystem::exists(video_path)) {
        error = "Video file not found: " + video_path;
        return false;
    }
    if (!std::filesystem::exists(audio_path)) {
        error = "Audio file not found: " + audio_path;
        return false;
    }

    std::filesystem::path video_p(video_path);
    std::filesystem::path temp_video_p = video_p.parent_path() / ("v_temp_" + video_p.filename().string());
    
    try {
        if (std::filesystem::exists(temp_video_p)) {
            std::filesystem::remove(temp_video_p);
        }
        std::filesystem::rename(video_p, temp_video_p);
    } catch (const std::exception& e) {
        error = std::string("Failed to create temporary video file: ") + e.what();
        return false;
    }

    // Use the robust command builder from output_utils
    // Note: build_ffmpeg_mux_command uses plan.output.destination.path as output
    std::string command = output::build_ffmpeg_mux_command(plan, temp_video_p, audio_path);

    using namespace tachyon::core::platform;
    ProcessSpec spec;
#ifdef _WIN32
    spec.executable = "cmd.exe";
    spec.args = {"/C", command};
#else
    spec.executable = "sh";
    spec.args = {"-c", command};
#endif

    auto proc_result = run_process(spec);
    if (!proc_result.success || proc_result.exit_code != 0) {
        error = "FFmpeg mux failed with code: " + std::to_string(proc_result.exit_code);
        // Try to restore the original video if muxing failed
        try {
            if (std::filesystem::exists(temp_video_p)) {
                std::filesystem::rename(temp_video_p, video_p);
            }
        } catch (...) {}
        return false;
    }

    try {
        if (std::filesystem::exists(temp_video_p)) {
            std::filesystem::remove(temp_video_p);
        }
    } catch (const std::exception& e) {
        // Not fatal, but good to know
        error = std::string("Failed to cleanup temporary video: ") + e.what();
    }

    return true;
}

} // namespace tachyon
