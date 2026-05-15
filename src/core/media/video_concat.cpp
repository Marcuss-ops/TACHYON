#include "tachyon/core/media/video_concat.h"
#include "tachyon/core/platform/process.h"
#include <fstream>

namespace tachyon::core::media {

MediaResult<void> VideoConcat::concat_videos(const ConcatConfig& config) {
    if (config.inputs.empty()) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::IO, "No input files provided for concatenation")
        );
    }

    // Create temporary concat file
    std::filesystem::path temp_list = config.output;
    temp_list.replace_extension(".concat.txt");
    
    std::ofstream list_file(temp_list);
    if (!list_file.is_open()) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::IO, "Failed to create temporary concat list")
                .with_path(temp_list.string())
        );
    }

    for (const auto& input : config.inputs) {
        if (!std::filesystem::exists(input)) {
             return MediaResult<void>::failure(
                MediaError(MediaErrorCode::IO, "Input file missing")
                    .with_path(input.string())
            );
        }
        // FFmpeg concat demuxer requires 'file 'path'' format
        list_file << "file '" << input.string() << "'\n";
    }
    list_file.close();

    std::vector<std::string> args;
    args.push_back("-y");
    args.push_back("-f");
    args.push_back("concat");
    args.push_back("-safe");
    args.push_back("0");
    args.push_back("-i");
    args.push_back(temp_list.string());
    
    args.push_back("-c:v");
    args.push_back(config.video_codec);
    args.push_back("-crf");
    args.push_back(std::to_string(config.crf));
    args.push_back("-preset");
    args.push_back("fast");
    
    if (config.use_aac) {
        args.push_back("-c:a");
        args.push_back("aac");
        args.push_back("-b:a");
        args.push_back("256k");
    } else {
        args.push_back("-c:a");
        args.push_back("copy");
    }
    
    args.push_back("-movflags");
    args.push_back("+faststart");
    
    args.push_back(config.output.string());

    platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    spec.timeout = std::chrono::minutes(10);
    
    auto result = platform::run_process(spec);
    
    // Cleanup temp file
    std::error_code ec;
    std::filesystem::remove(temp_list, ec);

    if (!result.success || result.exit_code != 0) {
        return MediaResult<void>::failure(
            MediaError(MediaErrorCode::Encode, "FFmpeg concatenation failed: " + result.error)
                .with_stage("concat_videos")
        );
    }

    return MediaResult<void>::success();
}

} // namespace tachyon::core::media
