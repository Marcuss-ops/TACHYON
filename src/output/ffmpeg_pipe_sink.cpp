#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color_transfer.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
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

std::string ffmpeg_color_primaries(std::string_view value) {
    const auto primaries = renderer2d::detail::parse_color_primaries(value);
    switch (primaries) {
    case renderer2d::detail::ColorPrimaries::Bt709:
        return "bt709";
    case renderer2d::detail::ColorPrimaries::DciP3:
        return "smpte432";
    case renderer2d::detail::ColorPrimaries::Rec2020:
        return "bt2020";
    case renderer2d::detail::ColorPrimaries::AcesAP1:
        return "unknown";
    case renderer2d::detail::ColorPrimaries::AcesAP0:
        return "unknown";
    case renderer2d::detail::ColorPrimaries::Srgb:
    default:
        return "bt709";
    }
}

std::string ffmpeg_transfer_characteristics(std::string_view value) {
    const auto curve = renderer2d::detail::parse_transfer_curve(value);
    switch (curve) {
    case renderer2d::detail::TransferCurve::Linear:
        return "linear";
    case renderer2d::detail::TransferCurve::Bt709:
        return "bt709";
    case renderer2d::detail::TransferCurve::Srgb:
    default:
        return "iec61966-2-1";
    }
}

std::string ffmpeg_colorspace(std::string_view value) {
    const auto primaries = renderer2d::detail::parse_color_primaries(value);
    switch (primaries) {
    case renderer2d::detail::ColorPrimaries::Rec2020:
        return "bt2020nc";
    case renderer2d::detail::ColorPrimaries::DciP3:
    case renderer2d::detail::ColorPrimaries::Srgb:
    case renderer2d::detail::ColorPrimaries::Bt709:
    case renderer2d::detail::ColorPrimaries::AcesAP1:
    case renderer2d::detail::ColorPrimaries::AcesAP0:
    default:
        return "bt709";
    }
}

std::string ffmpeg_color_range(std::string_view value) {
    const auto range = renderer2d::detail::parse_color_range(value);
    return range == renderer2d::detail::ColorRange::Limited ? "tv" : "pc";
}

std::string build_ffmpeg_command(const RenderPlan& plan) {
    const double fps = plan.composition.frame_rate.value();
    const std::filesystem::path destination = std::filesystem::path(plan.output.destination.path);
    const bool overwrite = plan.output.destination.overwrite;
    const std::string pixel_format = resolve_output_pixel_format(plan.output);
    const std::string color_primaries = ffmpeg_color_primaries(plan.output.profile.color.space);
    const std::string color_trc = ffmpeg_transfer_characteristics(plan.output.profile.color.transfer);
    const std::string colorspace = ffmpeg_colorspace(plan.output.profile.color.space);
    const std::string color_range = ffmpeg_color_range(plan.output.profile.color.range);

    std::ostringstream command;
    command << "ffmpeg "
            << (overwrite ? "-y" : "-n")
            << " -f rawvideo"
            << " -pix_fmt rgba"
            << " -s " << plan.composition.width << 'x' << plan.composition.height
            << " -r " << fps
            << " -i -";

    // Multi-track Audio (Musk-YouTube Edition)
    const auto& tracks = plan.output.profile.audio.tracks;
    for (const auto& track : tracks) {
        if (!track.source_path.empty()) {
            command << " -i " << quote_path(track.source_path);
        }
    }

    command << " -color_primaries " << color_primaries
            << " -color_trc " << color_trc
            << " -colorspace " << colorspace
            << " -color_range " << color_range;

    if (!plan.output.profile.video.codec.empty()) {
        command << " -c:v " << plan.output.profile.video.codec;
    } else {
        command << " -c:v libx264";
    }

    if (plan.output.profile.video.rate_control_mode == "crf" && plan.output.profile.video.crf.has_value()) {
        command << " -crf " << *plan.output.profile.video.crf;
    }

    if (!pixel_format.empty()) {
        command << " -pix_fmt " << pixel_format;
    }

    // Filter Complex for Audio Mixing
    if (!tracks.empty()) {
        command << " -filter_complex \"";
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const auto& track = tracks[i];
            const std::int64_t delay_ms = static_cast<std::int64_t>(track.start_offset_seconds * 1000.0);
            command << "[" << (i + 1) << ":a]adelay=" << delay_ms << "|" << delay_ms 
                    << ",volume=" << track.volume << "[a" << i << "];";
        }
        
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            command << "[a" << i << "]";
        }
        command << "amix=inputs=" << tracks.size() << ":duration=first[outa]\" -map 0:v -map \"[outa]\"";
        command << " -c:a aac -b:a 192k -shortest";
    } else {
        command << " -an";
    }

    if (plan.output.profile.container == "mp4") {
        command << " -movflags +faststart";
    }

    command << ' ' << quote_path(destination);
    return command.str();
}

std::vector<unsigned char> convert_and_pack_frame_bytes(
    const renderer2d::Framebuffer& frame,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::ColorSpace source_space,
    renderer2d::detail::TransferCurve output_curve,
    renderer2d::detail::ColorSpace output_space,
    renderer2d::detail::ColorRange output_range) {

    std::vector<unsigned char> bytes;
    bytes.reserve(static_cast<std::size_t>(frame.width()) * static_cast<std::size_t>(frame.height()) * 4U);

    for (std::uint32_t y = 0; y < frame.height(); ++y) {
        for (std::uint32_t x = 0; x < frame.width(); ++x) {
            auto pixel = renderer2d::detail::convert_color(
                frame.get_pixel(x, y),
                source_curve, source_space,
                output_curve, output_space);
            pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
            bytes.push_back(pixel.r);
            bytes.push_back(pixel.g);
            bytes.push_back(pixel.b);
            bytes.push_back(pixel.a);
        }
    }

    return bytes;
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
        m_source_space = renderer2d::detail::parse_color_space(plan.working_space);
        m_output_transfer = renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer);
        m_output_space = renderer2d::detail::parse_color_space(plan.output.profile.color.space);
        m_output_range = renderer2d::detail::parse_color_range(plan.output.profile.color.range);

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

        const std::vector<unsigned char> bytes = convert_and_pack_frame_bytes(
            *packet.frame,
            m_source_transfer, m_source_space,
            m_output_transfer, m_output_space,
            m_output_range);
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
    renderer2d::detail::ColorSpace m_source_space{renderer2d::detail::ColorSpace::Srgb};
    renderer2d::detail::TransferCurve m_output_transfer{renderer2d::detail::TransferCurve::Srgb};
    renderer2d::detail::ColorSpace m_output_space{renderer2d::detail::ColorSpace::Srgb};
    renderer2d::detail::ColorRange m_output_range{renderer2d::detail::ColorRange::Full};
    std::string m_last_error;
};

} // namespace

std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink() {
    return std::make_unique<FfmpegPipeSink>();
}

} // namespace tachyon::output
