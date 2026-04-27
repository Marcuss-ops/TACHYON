#include <filesystem>
#include <string>
#include <cstdlib>
#include <system_error>

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

    int result = std::system(command.c_str());
    if (result != 0) {
        error = "FFmpeg mux failed with code: " + std::to_string(result);
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
