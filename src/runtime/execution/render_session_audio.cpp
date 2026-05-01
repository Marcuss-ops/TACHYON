#include <filesystem>
#include <string>
#include <cstdlib>
#include <system_error>

#include "tachyon/core/platform/process.h"

namespace tachyon {

bool mux_audio_video(const std::string& video_path, const std::string& audio_path, std::string& error) {
    if (!std::filesystem::exists(video_path)) {
        error = "Video file not found: " + video_path;
        return false;
    }
    if (!std::filesystem::exists(audio_path)) {
        error = "Audio file not found: " + audio_path;
        return false;
    }

    std::filesystem::path video_p(video_path);
    std::filesystem::path output_path = video_p.parent_path() / ("muxed_" + video_p.filename().string());

    std::string command = "ffmpeg -y -i \"" + video_path + "\" -i \"" + audio_path + "\" -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 \"" + output_path.string() + "\"";

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
        return false;
    }

    try {
        std::filesystem::remove(video_path);
        std::filesystem::rename(output_path, video_path);
    } catch (const std::exception& e) {
        error = std::string("Failed to replace video file: ") + e.what();
        return false;
    }

    return true;
}

} // namespace tachyon
