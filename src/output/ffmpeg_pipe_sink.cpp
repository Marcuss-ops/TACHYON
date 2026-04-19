#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color_transfer.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <stdio.h>
#define TACHYON_POPEN _popen
#define TACHYON_PCLOSE _pclose
#else
#define TACHYON_POPEN popen
#define TACHYON_PCLOSE pclose
#endif

namespace tachyon::output {
namespace {

std::string quote_path(const std::filesystem::path& path) {
    const std::string value = path.string();
    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('"');
    for (char ch : value) {
        if (ch == '"') {
            quoted.push_back('\\');
        }
        quoted.push_back(ch);
    }
    quoted.push_back('"');
    return quoted;
}

std::string resolve_output_pixel_format(const OutputContract& contract) {
    if (contract.profile.video.pixel_format == "rgba8") {
        return "rgba";
    }
    return contract.profile.video.pixel_format.empty() ? "yuv420p" : contract.profile.video.pixel_format;
}

std::string build_ffmpeg_command(const RenderPlan& plan) {
    const double fps = plan.composition.frame_rate.value();
    const std::filesystem::path destination = std::filesystem::path(plan.output.destination.path);
    const bool overwrite = plan.output.destination.overwrite;
    const std::string pixel_format = resolve_output_pixel_format(plan.output);

    std::ostringstream command;
    command << "ffmpeg "
            << (overwrite ? "-y" : "-n")
            << " -f rawvideo"
            << " -pix_fmt rgba"
            << " -s " << plan.composition.width << 'x' << plan.composition.height
            << " -r " << fps
            << " -i -"
            << " -an";

    if (!plan.output.profile.video.codec.empty()) {
        command << " -c:v " << plan.output.profile.video.codec;
    }

    if (plan.output.profile.video.rate_control_mode == "crf" && plan.output.profile.video.crf.has_value()) {
        command << " -crf " << *plan.output.profile.video.crf;
    }

    if (!pixel_format.empty()) {
        command << " -pix_fmt " << pixel_format;
    }

    if (plan.output.profile.container == "mp4") {
        command << " -movflags +faststart";
    }

    command << ' ' << quote_path(destination);
    return command.str();
}

std::vector<unsigned char> pack_frame_bytes(const renderer2d::Framebuffer& frame) {
    std::vector<unsigned char> bytes;
    bytes.reserve(static_cast<std::size_t>(frame.width()) * static_cast<std::size_t>(frame.height()) * 4U);

    for (std::uint32_t packed : frame.pixels()) {
        bytes.push_back(static_cast<unsigned char>(packed & 0xFFU));
        bytes.push_back(static_cast<unsigned char>((packed >> 8U) & 0xFFU));
        bytes.push_back(static_cast<unsigned char>((packed >> 16U) & 0xFFU));
        bytes.push_back(static_cast<unsigned char>((packed >> 24U) & 0xFFU));
    }

    return bytes;
}

renderer2d::Framebuffer convert_frame(
    const renderer2d::Framebuffer& frame,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::TransferCurve output_curve) {

    if (source_curve == output_curve) {
        return frame;
    }

    renderer2d::Framebuffer converted(frame.width(), frame.height());
    for (std::uint32_t y = 0; y < frame.height(); ++y) {
        for (std::uint32_t x = 0; x < frame.width(); ++x) {
            converted.set_pixel(x, y, renderer2d::detail::convert_color_transfer(
                frame.get_pixel(x, y),
                source_curve,
                output_curve));
        }
    }
    return converted;
}

class FfmpegPipeSink final : public FrameOutputSink {
public:
    ~FfmpegPipeSink() override {
        if (m_pipe != nullptr) {
            TACHYON_PCLOSE(m_pipe);
            m_pipe = nullptr;
        }
    }

    bool begin(const RenderPlan& plan) override {
        m_last_error.clear();
        m_plan = &plan;
        m_source_transfer = renderer2d::detail::parse_transfer_curve(plan.working_space);
        m_output_transfer = renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer);

        if (plan.output.destination.path.empty()) {
            m_last_error = "ffmpeg output requires a destination path";
            return false;
        }

        if (plan.composition.width <= 0 || plan.composition.height <= 0) {
            m_last_error = "ffmpeg output requires a positive composition size";
            return false;
        }

        if (!plan.output.profile.audio.mode.empty() && plan.output.profile.audio.mode != "none") {
            m_last_error = "ffmpeg pipe sink currently supports audio.mode = none only";
            return false;
        }

        const std::filesystem::path destination(plan.output.destination.path);
        if (!destination.parent_path().empty()) {
            std::filesystem::create_directories(destination.parent_path());
        }

        const std::string command = build_ffmpeg_command(plan);
        m_pipe = TACHYON_POPEN(command.c_str(), "wb");
        if (m_pipe == nullptr) {
            m_last_error = "failed to open ffmpeg pipe";
            return false;
        }

        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        m_last_error.clear();
        if (m_pipe == nullptr) {
            m_last_error = "ffmpeg pipe sink is not open";
            return false;
        }
        if (packet.frame == nullptr) {
            m_last_error = "ffmpeg pipe sink received a null frame";
            return false;
        }

        const renderer2d::Framebuffer converted = convert_frame(*packet.frame, m_source_transfer, m_output_transfer);
        const std::vector<unsigned char> bytes = pack_frame_bytes(converted);
        const std::size_t written = std::fwrite(bytes.data(), 1, bytes.size(), m_pipe);
        if (written != bytes.size()) {
            m_last_error = "failed to write frame bytes to ffmpeg";
            return false;
        }

        return true;
    }

    bool finish() override {
        m_last_error.clear();
        if (m_pipe == nullptr) {
            return true;
        }

        const int status = TACHYON_PCLOSE(m_pipe);
        m_pipe = nullptr;
        if (status != 0) {
            m_last_error = "ffmpeg exited with a non-zero status";
            return false;
        }

        return true;
    }

    const std::string& last_error() const override {
        return m_last_error;
    }

private:
    const RenderPlan* m_plan{nullptr};
    FILE* m_pipe{nullptr};
    renderer2d::detail::TransferCurve m_source_transfer{renderer2d::detail::TransferCurve::Srgb};
    renderer2d::detail::TransferCurve m_output_transfer{renderer2d::detail::TransferCurve::Srgb};
    std::string m_last_error;
};

} // namespace

std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink() {
    return std::make_unique<FfmpegPipeSink>();
}

} // namespace tachyon::output
