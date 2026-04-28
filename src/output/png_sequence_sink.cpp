#include "tachyon/output/frame_output_sink.h"
#include "tachyon/output/prores_sink.h"
#include "tachyon/renderer2d/color/color_management_system.h"
#include "tachyon/renderer2d/color/color_transfer.h"

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
        
        // Initialize CMS for output conversion
        m_cms = renderer2d::ColorManagementSystem::default_pipeline();
        
        // Map RenderPlan working space string to ColorProfile
        // (Simplified mapping for now; in a full impl this would be more robust)
        if (plan.working_space == "srgb") {
            m_cms.working_profile = renderer2d::ColorProfile::sRGB();
        } else if (plan.working_space == "linear_rec709" || plan.working_space == "rec709") {
            m_cms.working_profile = renderer2d::ColorProfile::Rec709();
            m_cms.working_profile.curve = renderer2d::TransferCurve::Linear;
        } else if (plan.working_space == "acescg") {
            m_cms.working_profile = renderer2d::ColorProfile::ACEScg();
        }

        // Map OutputContract profiles to ColorProfile
        m_cms.output_profile.primaries = renderer2d::parse_color_primaries(plan.output.profile.color.space);
        m_cms.output_profile.curve = renderer2d::parse_transfer_curve(plan.output.profile.color.transfer);

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

        // Apply colour management: Working -> Output
        renderer2d::Framebuffer converted = *packet.frame;
        auto graph = m_cms.build_working_to_output();
        graph.process_surface(converted, m_cms.output_profile);

        // Save as PNG (save_png will handle the final transfer curve if needed, 
        // but CMS already applied it in build_working_to_output if output_profile.curve != Linear)
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

    std::filesystem::path m_destination;
    bool m_overwrite{false};
    std::size_t m_next_index{1};
    renderer2d::ColorManagementSystem m_cms;
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

    const auto& profile = plan.output.profile;
    const std::string& container = profile.container;
    const std::string& codec = profile.video.codec;

    // ProRes: MOV container with prores codec
    if (container == "mov" && codec.find("prores") != std::string::npos) {
        ProResSinkConfig config;
        config.profile = codec;
        return create_prores_sink(config);
    }

    // DNxHD: MOV container with dnxhd codec
    if (container == "mov" && codec.find("dnxhd") != std::string::npos) {
        // TODO: Create DNxHD sink
        return nullptr;
    }

    // GIF: gif container
    if (container == "gif") {
        // TODO: Create GIF sink
        return nullptr;
    }

    // WebM: webm container
    if (container == "webm") {
        // TODO: Create WebM sink
        return nullptr;
    }

    if (output_requests_video_file(plan.output)) {
        return create_ffmpeg_pipe_sink();
    }

    return create_png_sequence_sink();
}

} // namespace tachyon::output
