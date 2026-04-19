#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color_transfer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>

namespace tachyon::output {
namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string extension_lower(const std::filesystem::path& path) {
    return to_lower(path.extension().string());
}

bool is_video_extension(const std::string& extension) {
    return extension == ".mp4" || extension == ".mov" || extension == ".mkv" || extension == ".webm";
}

std::filesystem::path resolve_frame_path(const std::filesystem::path& destination, std::size_t output_index) {
    std::filesystem::path directory = destination;
    std::string stem = "frame";

    if (destination.has_extension()) {
        directory = destination.parent_path();
        stem = destination.stem().string();
        if (stem.empty()) {
            stem = "frame";
        }
    }

    if (directory.empty()) {
        directory = ".";
    }

    std::ostringstream filename;
    filename << stem << '_' << std::setw(6) << std::setfill('0') << output_index << ".png";
    return directory / filename.str();
}

class PngSequenceSink final : public FrameOutputSink {
public:
    bool begin(const RenderPlan& plan) override {
        m_last_error.clear();
        m_destination = std::filesystem::path(plan.output.destination.path);
        m_overwrite = plan.output.destination.overwrite;
        m_source_transfer = renderer2d::detail::parse_transfer_curve(plan.working_space);
        m_source_space = renderer2d::detail::parse_color_space(plan.working_space);
        m_output_transfer = renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer);
        m_output_space = renderer2d::detail::parse_color_space(plan.output.profile.color.space);
        m_output_range = renderer2d::detail::parse_color_range(plan.output.profile.color.range);
        m_next_index = 1;

        if (m_destination.empty()) {
            m_last_error = "png sequence output requires a destination path";
            return false;
        }

        const std::filesystem::path first_frame_path = resolve_frame_path(m_destination, m_next_index);
        if (!first_frame_path.parent_path().empty()) {
            std::filesystem::create_directories(first_frame_path.parent_path());
        }

        if (!m_overwrite && std::filesystem::exists(first_frame_path)) {
            m_last_error = "png sequence output exists and overwrite is disabled: " + first_frame_path.string();
            return false;
        }

        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        m_last_error.clear();
        if (packet.frame == nullptr) {
            m_last_error = "png sequence sink received a null frame";
            return false;
        }

        const std::filesystem::path frame_path = resolve_frame_path(m_destination, m_next_index);
        if (!m_overwrite && std::filesystem::exists(frame_path)) {
            m_last_error = "png sequence frame exists and overwrite is disabled: " + frame_path.string();
            return false;
        }

        renderer2d::Framebuffer converted = convert_frame(
            *packet.frame,
            m_source_transfer, m_source_space,
            m_output_transfer, m_output_space,
            m_output_range);
        if (!converted.save_png(frame_path)) {
            m_last_error = "failed to write png frame: " + frame_path.string();
            return false;
        }

        ++m_next_index;
        return true;
    }

    bool finish() override {
        return m_last_error.empty();
    }

    const std::string& last_error() const override {
        return m_last_error;
    }

private:
    static renderer2d::Framebuffer convert_frame(
        const renderer2d::Framebuffer& frame,
        renderer2d::detail::TransferCurve source_curve,
        renderer2d::detail::ColorSpace source_space,
        renderer2d::detail::TransferCurve output_curve,
        renderer2d::detail::ColorSpace output_space,
        renderer2d::detail::ColorRange output_range) {

        if (source_curve == output_curve && source_space == output_space && output_range == renderer2d::detail::ColorRange::Full) {
            return frame;
        }

        renderer2d::Framebuffer converted(frame.width(), frame.height());
        for (std::uint32_t y = 0; y < frame.height(); ++y) {
            for (std::uint32_t x = 0; x < frame.width(); ++x) {
                auto pixel = renderer2d::detail::convert_color(
                    frame.get_pixel(x, y),
                    source_curve, source_space,
                    output_curve, output_space);
                pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
                converted.set_pixel(x, y, pixel);
            }
        }
        return converted;
    }

    std::filesystem::path m_destination;
    bool m_overwrite{false};
    std::size_t m_next_index{1};
    renderer2d::detail::TransferCurve m_source_transfer{renderer2d::detail::TransferCurve::Srgb};
    renderer2d::detail::ColorSpace m_source_space{renderer2d::detail::ColorSpace::Srgb};
    renderer2d::detail::TransferCurve m_output_transfer{renderer2d::detail::TransferCurve::Srgb};
    renderer2d::detail::ColorSpace m_output_space{renderer2d::detail::ColorSpace::Srgb};
    renderer2d::detail::ColorRange m_output_range{renderer2d::detail::ColorRange::Full};
    std::string m_last_error;
};

} // namespace

bool output_requests_png_sequence(const OutputContract& contract) {
    const std::string class_name = to_lower(contract.profile.class_name);
    const std::string container = to_lower(contract.profile.container);
    const std::string extension = extension_lower(std::filesystem::path(contract.destination.path));

    if (class_name == "png_sequence" || class_name == "png-sequence" ||
        class_name == "image_sequence" || class_name == "image-sequence") {
        return true;
    }

    if (container == "png" || container == "image_sequence" || container == "image-sequence") {
        return true;
    }

    return extension == ".png" || (!extension.empty() && !is_video_extension(extension));
}

bool output_requests_video_file(const OutputContract& contract) {
    const std::string class_name = to_lower(contract.profile.class_name);
    const std::string container = to_lower(contract.profile.container);
    const std::string extension = extension_lower(std::filesystem::path(contract.destination.path));

    if (class_name == "ffmpeg_pipe" || class_name == "ffmpeg-pipe" ||
        class_name == "video" || class_name == "video_file" || class_name == "video-file") {
        return true;
    }

    if (container == "mp4" || container == "mov" || container == "mkv" || container == "webm") {
        return true;
    }

    return is_video_extension(extension);
}

std::unique_ptr<FrameOutputSink> create_png_sequence_sink() {
    return std::make_unique<PngSequenceSink>();
}

std::unique_ptr<FrameOutputSink> create_frame_output_sink(const RenderPlan& plan) {
    if (plan.output.destination.path.empty()) {
        return nullptr;
    }

    if (output_requests_video_file(plan.output)) {
        return create_ffmpeg_pipe_sink();
    }

    return create_png_sequence_sink();
}

} // namespace tachyon::output
