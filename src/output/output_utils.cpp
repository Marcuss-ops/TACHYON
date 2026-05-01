#include "tachyon/output/output_utils.h"
#include "tachyon/core/string_utils.h"

#include <filesystem>

namespace tachyon::output {
namespace {

std::string extension_lower(const std::filesystem::path& path) {
    return tachyon::ascii_lower(path.extension().string());
}

bool is_video_extension(const std::string& ext) {
    return ext == ".mp4" || ext == ".mov" || ext == ".mkv" || ext == ".webm";
}

bool is_exr_extension(const std::string& ext) {
    return ext == ".exr";
}

} // namespace

OutputTargetKind classify_output_contract(const OutputContract& contract) {
    const std::string class_name = tachyon::ascii_lower(contract.profile.class_name);
    const std::string container = tachyon::ascii_lower(contract.profile.container);
    const std::string extension = extension_lower(std::filesystem::path(contract.destination.path));

    if (
        class_name == "png_sequence" ||
        class_name == "png-sequence" ||
        class_name == "image_sequence" ||
        class_name == "image-sequence" ||
        container == "png" ||
        container == "image_sequence" ||
        container == "image-sequence" ||
        extension == ".png"
    ) {
        return OutputTargetKind::PngSequence;
    }

    if (
        class_name == "exr_sequence" ||
        class_name == "exr-sequence" ||
        container == "exr" ||
        is_exr_extension(extension)
    ) {
        return OutputTargetKind::ExrSequence;
    }

    if (
        class_name == "ffmpeg_pipe" ||
        class_name == "ffmpeg-pipe" ||
        class_name == "video" ||
        class_name == "video_file" ||
        class_name == "video-file" ||
        container == "mp4" ||
        container == "mov" ||
        container == "mkv" ||
        container == "webm" ||
        is_video_extension(extension)
    ) {
        return OutputTargetKind::VideoFile;
    }

    if (!extension.empty() && !is_video_extension(extension)) {
        return OutputTargetKind::PngSequence;
    }

    return OutputTargetKind::Unknown;
}

bool output_requests_png_sequence(const OutputContract& contract) {
    return classify_output_contract(contract) == OutputTargetKind::PngSequence;
}

bool output_requests_video_file(const OutputContract& contract) {
    return classify_output_contract(contract) == OutputTargetKind::VideoFile;
}

bool output_requests_exr(const OutputContract& contract) {
    return classify_output_contract(contract) == OutputTargetKind::ExrSequence;
}

} // namespace tachyon::output
