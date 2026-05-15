#include "tachyon/backends/ffmpeg/ffmpeg_video_concat.h"
#include "tachyon/core/platform/process.h"
#include <fstream>

namespace tachyon::backends::ffmpeg {

using namespace core::media;
using core::MediaResult;
using core::MediaError;
using core::MediaErrorCode;

MediaResult<void> FFmpegVideoConcat::concat_videos(const ConcatConfig& config) {
    if (config.inputs.empty()) {
        return MediaResult<void>::failure(MediaError(MediaErrorCode::IO, "No inputs for concatenation"));
    }

    // Create temporary concat list file
    std::filesystem::path list_path = config.output.parent_path() / "concat_list.txt";
    std::ofstream list_file(list_path);
    if (!list_file) {
        return MediaResult<void>::failure(MediaError(MediaErrorCode::IO, "Failed to create concat list file"));
    }

    for (const auto& input : config.inputs) {
        list_file << "file '" << std::filesystem::absolute(input).string() << "'\n";
    }
    list_file.close();

    std::vector<std::string> args = {
        "-y",
        "-f", "concat",
        "-safe", "0",
        "-i", list_path.string(),
        "-c:v", config.video_codec,
        "-crf", std::to_string(config.crf)
    };

    if (config.use_aac) {
        args.push_back("-c:a");
        args.push_back("aac");
    } else {
        args.push_back("-c:a");
        args.push_back("copy");
    }

    args.push_back(config.output.string());

    core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    spec.timeout = std::chrono::minutes(30);

    auto result = core::platform::run_process(spec);
    
    // Cleanup
    std::filesystem::remove(list_path);

    if (!result.success || result.exit_code != 0) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::Encode, "FFmpeg concatenation failed: " + result.error)
                .with_stage("concat")
        );
    }

    return MediaResult<void>::success();
}

} // namespace tachyon::backends::ffmpeg
