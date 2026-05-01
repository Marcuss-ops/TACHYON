#include "tachyon/output/ffmpeg_command.h"
#include "tachyon/core/platform/shell_escape.h"

#include <sstream>
#include <string>

namespace tachyon::output {

using namespace tachyon::core::platform;

FfmpegCommand& FfmpegCommand::overwrite(bool enable) {
    m_overwrite = enable;
    return *this;
}

FfmpegCommand& FfmpegCommand::input_path(const std::filesystem::path& path) {
    m_input_path = path;
    m_input_pipe = false;
    return *this;
}

FfmpegCommand& FfmpegCommand::input_pipe() {
    m_input_path.reset();
    m_input_pipe = true;
    return *this;
}

FfmpegCommand& FfmpegCommand::raw_video_input(int width, int height, double fps, const std::string& pix_fmt) {
    m_raw_video.enabled = true;
    m_raw_video.width = width;
    m_raw_video.height = height;
    m_raw_video.fps = fps;
    m_raw_video.pix_fmt = pix_fmt;
    m_input_pipe = true;
    return *this;
}

FfmpegCommand& FfmpegCommand::output_path(const std::filesystem::path& path) {
    m_output_path = path;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_codec(const std::string& codec) {
    m_video.codec = codec;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_bitrate_kbps(int bitrate_kbps) {
    m_video.bitrate_kbps = bitrate_kbps;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_crf(int crf) {
    m_video.crf = crf;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_preset(const std::string& preset) {
    m_video.preset = preset;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_pixel_format(const std::string& pix_fmt) {
    m_video.pixel_format = pix_fmt;
    return *this;
}

FfmpegCommand& FfmpegCommand::video_profile(const std::string& profile) {
    m_video.profile = profile;
    return *this;
}

FfmpegCommand& FfmpegCommand::audio_codec(const std::string& codec) {
    m_audio.codec = codec;
    return *this;
}

FfmpegCommand& FfmpegCommand::audio_bitrate_kbps(int bitrate_kbps) {
    m_audio.bitrate_kbps = bitrate_kbps;
    return *this;
}

FfmpegCommand& FfmpegCommand::add_filter(const std::string& filter) {
    m_filters.push_back(filter);
    return *this;
}

FfmpegCommand& FfmpegCommand::color_primaries(const std::string& primaries) {
    m_color_primaries = primaries;
    return *this;
}

FfmpegCommand& FfmpegCommand::color_transfer(const std::string& transfer) {
    m_color_transfer = transfer;
    return *this;
}

FfmpegCommand& FfmpegCommand::color_space(const std::string& space) {
    m_color_space = space;
    return *this;
}

FfmpegCommand& FfmpegCommand::color_range(const std::string& range) {
    m_color_range = range;
    return *this;
}

FfmpegCommand& FfmpegCommand::movflags_faststart(bool enable) {
    m_movflags_faststart = enable;
    return *this;
}

FfmpegCommand& FfmpegCommand::codec_copy() {
    m_codec_copy = true;
    return *this;
}

FfmpegCommand& FfmpegCommand::extra_arg(const std::string& arg) {
    m_extra_args.push_back(arg);
    return *this;
}

std::string FfmpegCommand::str() const {
    std::ostringstream cmd;
    cmd << "ffmpeg";

    if (m_overwrite) {
        cmd << " -y";
    } else {
        cmd << " -n";
    }

    cmd << build_input_args();
    cmd << build_output_args();

    return cmd.str();
}

std::string FfmpegCommand::build_input_args() const {
    std::ostringstream args;

    if (m_raw_video.enabled) {
        args << " -f rawvideo"
             << " -pix_fmt " << m_raw_video.pix_fmt
             << " -s " << m_raw_video.width << 'x' << m_raw_video.height
             << " -r " << m_raw_video.fps
             << " -i -";
    } else if (m_input_path.has_value()) {
        args << " -i " << quote_shell_arg(*m_input_path);
    } else if (m_input_pipe) {
        args << " -i -";
    }

    return args.str();
}

std::string FfmpegCommand::build_output_args() const {
    std::ostringstream args;

    if (m_codec_copy) {
        args << " -c:v copy";
    } else if (m_video.codec.has_value()) {
        args << " -c:v " << *m_video.codec;

        if (m_video.profile.has_value()) {
            args << " -profile:v " << *m_video.profile;
        }
        if (m_video.preset.has_value()) {
            args << " -preset " << *m_video.preset;
        }
        if (m_video.crf.has_value()) {
            args << " -crf " << *m_video.crf;
        }
        if (m_video.bitrate_kbps.has_value()) {
            args << " -b:v " << *m_video.bitrate_kbps << 'k';
        }
        if (m_video.pixel_format.has_value()) {
            args << " -pix_fmt " << *m_video.pixel_format;
        }
    }

    if (m_audio.codec.has_value()) {
        args << " -c:a " << *m_audio.codec;
        if (m_audio.bitrate_kbps.has_value()) {
            args << " -b:a " << *m_audio.bitrate_kbps << 'k';
        }
    }

    if (!m_filters.empty()) {
        args << " -vf \"";
        for (size_t i = 0; i < m_filters.size(); ++i) {
            if (i > 0) args << ',';
            args << m_filters[i];
        }
        args << '"';
    }

    if (m_color_primaries.has_value()) {
        args << " -color_primaries " << *m_color_primaries;
    }
    if (m_color_transfer.has_value()) {
        args << " -color_trc " << *m_color_transfer;
    }
    if (m_color_space.has_value()) {
        args << " -colorspace " << *m_color_space;
    }
    if (m_color_range.has_value()) {
        args << " -color_range " << *m_color_range;
    }

    if (m_movflags_faststart) {
        args << " -movflags +faststart";
    }

    for (const auto& arg : m_extra_args) {
        args << ' ' << arg;
    }

    if (m_output_path.has_value()) {
        args << ' ' << quote_shell_arg(*m_output_path);
    }

    return args.str();
}

} // namespace tachyon::output
