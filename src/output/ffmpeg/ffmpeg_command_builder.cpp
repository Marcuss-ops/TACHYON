#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <sstream>
#include <filesystem>
#include <chrono>

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

std::string ffmpeg_color_primaries(renderer2d::detail::ColorPrimaries primaries) {
    switch (primaries) {
    case renderer2d::detail::ColorPrimaries::Rec709:
        return "bt709";
    case renderer2d::detail::ColorPrimaries::P3D65:
        return "smpte432";
    default:
        return "bt709";
    }
}

std::string ffmpeg_transfer_characteristics(renderer2d::detail::TransferCurve curve) {
    switch (curve) {
    case renderer2d::detail::TransferCurve::Linear:
        return "linear";
    case renderer2d::detail::TransferCurve::Rec709:
        return "bt709";
    case renderer2d::detail::TransferCurve::sRGB:
    default:
        return "iec61966-2-1";
    }
}

std::string ffmpeg_colorspace(renderer2d::detail::ColorPrimaries primaries) {
    (void)primaries;
    return "bt709";
}

std::string ffmpeg_color_range(renderer2d::detail::ColorRange range) {
    return range == renderer2d::detail::ColorRange::Limited ? "tv" : "pc";
}

bool is_alpha_requested(const OutputProfile& profile) {
    const std::string lower{renderer2d::detail::ascii_lower(profile.alpha_mode)};
    return lower == "premultiplied" || lower == "straight" || lower == "unassociated";
}
} // namespace

std::string build_ffmpeg_video_command(const RenderPlan& plan, const std::filesystem::path& output_path, bool include_faststart) {
    const double fps = plan.composition.frame_rate.value();
    const bool overwrite = plan.output.destination.overwrite;
    const std::string color_primaries = ffmpeg_color_primaries(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_trc = ffmpeg_transfer_characteristics(renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer));
    const std::string colorspace = ffmpeg_colorspace(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_range = ffmpeg_color_range(renderer2d::detail::parse_color_range(plan.output.profile.color.range));

    std::ostringstream command;
    command << "ffmpeg "
            << (overwrite ? "-y" : "-n")
            << " -f rawvideo"
            << " -pix_fmt rgba"
            << " -s " << plan.composition.width << 'x' << plan.composition.height
            << " -r " << fps
            << " -i -"
            << " -an";

    const bool alpha = is_alpha_requested(plan.output.profile);

    if (plan.output.profile.format == OutputFormat::ProRes) {
        if (alpha) {
            command << " -c:v prores_ks -profile:v 4 -vendor apl0 -bits_per_mb 8000 -pix_fmt yuva444p10le";
        } else {
            command << " -c:v prores_ks -profile:v 3 -vendor apl0 -pix_fmt yuv422p10le";
        }
    } else if (plan.output.profile.format == OutputFormat::WebM) {
        command << " -c:v libvpx-vp9";
        if (alpha) {
            command << " -pix_fmt yuva420p -auto-alt-ref 0";
        } else {
            command << " -pix_fmt yuv420p";
        }
        if (!plan.output.profile.video.crf.has_value()) {
            command << " -crf 23 -b:v 0";
        }
    } else {
        if (!plan.output.profile.video.codec.empty()) {
            command << " -c:v " << plan.output.profile.video.codec;
        } else {
            command << " -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p";
        }

        if (!plan.output.profile.video.pixel_format.empty()) {
            command << " -pix_fmt " << plan.output.profile.video.pixel_format;
        } else if (plan.output.profile.video.codec == "libx264" || plan.output.profile.video.codec == "libx265") {
            command << " -pix_fmt yuv420p";
        }
    }

    if (plan.output.profile.video.rate_control_mode == "crf" && plan.output.profile.video.crf.has_value()) {
        command << " -crf " << *plan.output.profile.video.crf;
    }

    command << " -color_primaries " << color_primaries
            << " -color_trc " << color_trc
            << " -colorspace " << colorspace
            << " -color_range " << color_range;

    if (include_faststart && plan.output.profile.container == "mp4") {
        command << " -movflags +faststart";
    }

    command << ' ' << quote_path(output_path);
    return command.str();
}

std::string build_ffmpeg_mux_command(const RenderPlan& plan, const std::filesystem::path& video_path, const std::filesystem::path& audio_path) {
    const bool overwrite = plan.output.destination.overwrite;
    const std::string color_primaries = ffmpeg_color_primaries(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_trc = ffmpeg_transfer_characteristics(renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer));
    const std::string colorspace = ffmpeg_colorspace(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_range = ffmpeg_color_range(renderer2d::detail::parse_color_range(plan.output.profile.color.range));

    std::ostringstream command;
    command << "ffmpeg "
            << (overwrite ? "-y" : "-n")
            << " -i " << quote_path(video_path)
            << " -i " << quote_path(audio_path);

    command << " -color_primaries " << color_primaries
            << " -color_trc " << color_trc
            << " -colorspace " << colorspace
            << " -color_range " << color_range;

    command << " -c:v copy";

    if (!plan.output.profile.audio.codec.empty()) {
        command << " -c:a " << plan.output.profile.audio.codec;
    } else {
        command << " -c:a aac";
    }

    if (plan.output.profile.container == "mp4") {
        command << " -movflags +faststart";
    }

    command << " -shortest";
    command << ' ' << quote_path(std::filesystem::path(plan.output.destination.path));
    return command.str();
}

std::filesystem::path make_ffmpeg_temp_path(const std::filesystem::path& destination, const std::string& suffix, const std::string& extension) {
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::string stem = destination.stem().string().empty() ? "tachyon" : destination.stem().string();
    return temp_dir / (stem + ".tachyon." + suffix + "." + std::to_string(static_cast<long long>(stamp)) + extension);
}

} // namespace tachyon::output
