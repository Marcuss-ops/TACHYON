#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/audio/audio_mixer.h"
#include "tachyon/audio/audio_decoder.h"

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cmath>
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

bool is_alpha_requested(const OutputProfile& profile) {
    const std::string lower{renderer2d::detail::ascii_lower(profile.alpha_mode)};
    return lower == "premultiplied" || lower == "straight" || lower == "unassociated";
}

std::string resolve_output_pixel_format(const OutputContract& contract) {
    const bool alpha = is_alpha_requested(contract.profile);
    
    if (contract.profile.video.pixel_format == "rgba8") {
        return "rgba";
    }
    
    if (contract.profile.format == OutputFormat::ProRes) {
        return alpha ? "yuva444p10le" : "yuv422p10le";
    }
    
    if (contract.profile.format == OutputFormat::WebM) {
        return alpha ? "yuva420p" : "yuv420p";
    }
    
    return contract.profile.video.pixel_format.empty() ? "yuv420p" : contract.profile.video.pixel_format;
}

std::string ffmpeg_color_primaries(renderer2d::detail::ColorPrimaries primaries) {
    switch (primaries) {
    case renderer2d::detail::ColorPrimaries::Rec709:
        return "bt709";
    case renderer2d::detail::ColorPrimaries::DisplayP3:
        return "smpte432";
    case renderer2d::detail::ColorPrimaries::sRGB:
    case renderer2d::detail::ColorPrimaries::Linear:
    default:
        return "bt709";
    }
}

std::string ffmpeg_transfer_characteristics(renderer2d::detail::TransferCurve curve) {
    switch (curve) {
    case renderer2d::detail::TransferCurve::Linear:
        return "linear";
    case renderer2d::detail::TransferCurve::Bt709:
        return "bt709";
    case renderer2d::detail::TransferCurve::sRGB:
    default:
        return "iec61966-2-1";
    }
}

std::string ffmpeg_colorspace(renderer2d::detail::ColorPrimaries primaries) {
    switch (primaries) {
    case renderer2d::detail::ColorPrimaries::Rec709:
    case renderer2d::detail::ColorPrimaries::DisplayP3:
    case renderer2d::detail::ColorPrimaries::sRGB:
    case renderer2d::detail::ColorPrimaries::Linear:
    default:
        return "bt709";
    }
}

std::string ffmpeg_color_range(renderer2d::detail::ColorRange range) {
    return range == renderer2d::detail::ColorRange::Limited ? "tv" : "pc";
}

std::filesystem::path make_temp_video_path(const std::filesystem::path& destination) {
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::string stem = destination.stem().string().empty() ? "tachyon" : destination.stem().string();
    return temp_dir / (stem + ".tachyon.video." + std::to_string(static_cast<long long>(stamp)) + ".mkv");
}

std::filesystem::path make_temp_palette_path(const std::filesystem::path& destination) {
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::string stem = destination.stem().string().empty() ? "tachyon" : destination.stem().string();
    return temp_dir / (stem + ".tachyon.palette." + std::to_string(static_cast<long long>(stamp)) + ".png");
}

std::string build_video_pass_command(const RenderPlan& plan, const std::filesystem::path& output_path, bool include_faststart) {
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

std::string build_audio_mux_command(const RenderPlan& plan, const std::filesystem::path& temp_video_path, const std::filesystem::path& master_audio_path) {
    const bool overwrite = plan.output.destination.overwrite;
    const std::string color_primaries = ffmpeg_color_primaries(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_trc = ffmpeg_transfer_characteristics(renderer2d::detail::parse_transfer_curve(plan.output.profile.color.transfer));
    const std::string colorspace = ffmpeg_colorspace(renderer2d::detail::parse_color_primaries(plan.output.profile.color.space));
    const std::string color_range = ffmpeg_color_range(renderer2d::detail::parse_color_range(plan.output.profile.color.range));

    std::ostringstream command;
    command << "ffmpeg "
            << (overwrite ? "-y" : "-n")
            << " -i " << quote_path(temp_video_path)
            << " -i " << quote_path(master_audio_path);

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

std::vector<unsigned char> convert_and_pack_frame_bytes(
    const renderer2d::Framebuffer& frame,
    uint32_t target_width,
    uint32_t target_height,
    renderer2d::detail::TransferCurve source_curve,
    renderer2d::detail::ColorSpace source_space,
    renderer2d::detail::TransferCurve output_curve,
    renderer2d::detail::ColorSpace output_space,
    renderer2d::detail::ColorRange output_range) {

    const uint32_t src_w = frame.width();
    const uint32_t src_h = frame.height();
    std::vector<unsigned char> bytes;
    bytes.reserve(static_cast<std::size_t>(target_width) * static_cast<std::size_t>(target_height) * 4U);

    const float scale_x = static_cast<float>(src_w) / static_cast<float>(target_width);
    const float scale_y = static_cast<float>(src_h) / static_cast<float>(target_height);

    for (std::uint32_t y = 0; y < target_height; ++y) {
        for (std::uint32_t x = 0; x < target_width; ++x) {
            renderer2d::Color pixel;
            
            if (src_w == target_width && src_h == target_height) {
                pixel = frame.get_pixel(x, y);
            } else {
                // Bilinear Upscale
                const float gx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
                const float gy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;
                const int gxi = static_cast<int>(std::floor(gx));
                const int gyi = static_cast<int>(std::floor(gy));
                const float tx = gx - static_cast<float>(gxi);
                const float ty = gy - static_cast<float>(gyi);

                auto sample = [&](int sx, int sy) {
                    return frame.get_pixel(
                        static_cast<uint32_t>(std::clamp(sx, 0, static_cast<int>(src_w) - 1)),
                        static_cast<uint32_t>(std::clamp(sy, 0, static_cast<int>(src_h) - 1))
                    );
                };

                const auto c00 = sample(gxi, gyi);
                const auto c10 = sample(gxi + 1, gyi);
                const auto c01 = sample(gxi, gyi + 1);
                const auto c11 = sample(gxi + 1, gyi + 1);

                const auto top = renderer2d::Color::lerp(c00, c10, tx);
                const auto bottom = renderer2d::Color::lerp(c01, c11, tx);
                pixel = renderer2d::Color::lerp(top, bottom, ty);
            }

            pixel = renderer2d::detail::convert_color(
                pixel,
                source_curve, source_space,
                output_curve, output_space);
            pixel = renderer2d::detail::apply_range_mode(pixel, output_range);
            
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.r, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.g, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.b, 0.0f, 255.0f)));
            bytes.push_back(static_cast<unsigned char>(std::clamp(pixel.a, 0.0f, 255.0f)));
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
        if (!m_temp_video_path.empty()) {
            std::error_code ec;
            std::filesystem::remove(m_temp_video_path, ec);
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

        m_needs_audio_mux = !plan.output.profile.audio.mode.empty() &&
                            plan.output.profile.audio.mode != "none" &&
                            !plan.output.profile.audio.tracks.empty();
        
        bool needs_temp_video = m_needs_audio_mux || plan.output.profile.format == OutputFormat::Gif;

        const std::filesystem::path destination(plan.output.destination.path);
        if (!destination.parent_path().empty()) {
            std::filesystem::create_directories(destination.parent_path());
        }

        if (needs_temp_video) {
            m_temp_video_path = make_temp_video_path(destination);
        }

        const std::string command = needs_temp_video
            ? build_video_pass_command(plan, m_temp_video_path, false)
            : build_video_pass_command(plan, destination, true);
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
            static_cast<uint32_t>(m_plan->composition.width),
            static_cast<uint32_t>(m_plan->composition.height),
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
            return finalize_post_processing();
        }

        const int status = TACHYON_PCLOSE(m_pipe);
        m_pipe = nullptr;
        if (status != 0) {
            m_last_error = "ffmpeg exited with a non-zero status";
            return false;
        }

        return finalize_post_processing();
    }

    const std::string& last_error() const override {
        return m_last_error;
    }

private:
    bool finalize_gif() {
        if (m_plan == nullptr) {
            m_last_error = "gif finalization requires a render plan";
            return false;
        }

        const std::filesystem::path destination(m_plan->output.destination.path);
        const std::filesystem::path palette_path = make_temp_palette_path(destination);
        const bool overwrite = m_plan->output.destination.overwrite;

        std::ostringstream pass1;
        pass1 << "ffmpeg "
              << (overwrite ? "-y" : "-n")
              << " -i " << quote_path(m_temp_video_path)
              << " -vf \"palettegen=max_colors=256:stats_mode=full\""
              << " " << quote_path(palette_path);

        FILE* p1 = TACHYON_POPEN(pass1.str().c_str(), "rb");
        if (p1 == nullptr) {
            m_last_error = "gif pass 1 (palettegen): failed to launch ffmpeg";
            return false;
        }
        const int p1_status = TACHYON_PCLOSE(p1);
        if (p1_status != 0) {
            m_last_error = "gif pass 1 (palettegen): ffmpeg exited with error";
            return false;
        }

        std::ostringstream pass2;
        pass2 << "ffmpeg "
              << (overwrite ? "-y" : "-n")
              << " -i " << quote_path(m_temp_video_path)
              << " -i " << quote_path(palette_path)
              << " -lavfi \"paletteuse=dither=bayer\""
              << " " << quote_path(destination);

        FILE* p2 = TACHYON_POPEN(pass2.str().c_str(), "rb");
        if (p2 == nullptr) {
            m_last_error = "gif pass 2 (paletteuse): failed to launch ffmpeg";
            return false;
        }
        const int p2_status = TACHYON_PCLOSE(p2);
        if (p2_status != 0) {
            m_last_error = "gif pass 2 (paletteuse): ffmpeg exited with error";
            return false;
        }

        std::error_code ec;
        std::filesystem::remove(m_temp_video_path, ec);
        std::filesystem::remove(palette_path, ec);
        m_temp_video_path.clear();
        return true;
    }

    bool finalize_post_processing() {
        if (m_plan == nullptr) {
            return true;
        }
        if (m_plan->output.profile.format == OutputFormat::Gif) {
            return finalize_gif();
        }
        if (m_needs_audio_mux) {
            return finalize_audio_mux();
        }
        return true;
    }

    bool finalize_audio_mux() {
        if (m_plan == nullptr) {
            m_last_error = "ffmpeg mux finalization requires a render plan";
            return false;
        }

        const std::filesystem::path destination(m_plan->output.destination.path);
        const std::filesystem::path master_audio_path = destination.parent_path() / (destination.stem().string() + ".tachyon.master_audio.wav");

        audio::AudioMixer mixer;
        for (const auto& track_spec : m_plan->output.profile.audio.tracks) {
            if (track_spec.source_path.empty()) continue;
            auto decoder = std::make_shared<audio::AudioDecoder>();
            if (decoder->open(track_spec.source_path)) {
                mixer.add_track(decoder, {track_spec.start_offset_seconds, static_cast<float>(track_spec.volume)});
            }
        }

        std::ostringstream audio_cmd;
        audio_cmd << "ffmpeg -y -f f32le -ar 48000 -ac 2 -i - " << quote_path(master_audio_path);
        FILE* audio_pipe = TACHYON_POPEN(audio_cmd.str().c_str(), "wb");
        if (!audio_pipe) {
            m_last_error = "failed to open ffmpeg for master audio mix";
            return false;
        }

        const double duration = m_plan->composition.duration;
        const double chunk_duration = 1.0;
        std::vector<float> mix_buffer;
        for (double t = 0.0; t < duration; t += chunk_duration) {
            double current_chunk = std::min(chunk_duration, duration - t);
            mixer.mix(t, current_chunk, mix_buffer);
            std::fwrite(mix_buffer.data(), sizeof(float), mix_buffer.size(), audio_pipe);
        }
        TACHYON_PCLOSE(audio_pipe);

        const std::string mux_command = build_audio_mux_command(*m_plan, m_temp_video_path, master_audio_path);
        FILE* mux_pipe = TACHYON_POPEN(mux_command.c_str(), "wb");
        if (mux_pipe == nullptr) {
            m_last_error = "failed to open ffmpeg audio mux pass";
            return false;
        }

        const int mux_status = TACHYON_PCLOSE(mux_pipe);
        if (mux_status != 0) {
            m_last_error = "ffmpeg audio mux pass exited with a non-zero status";
            return false;
        }

        std::error_code ec;
        std::filesystem::remove(m_temp_video_path, ec);
        std::filesystem::remove(master_audio_path, ec);
        m_temp_video_path.clear();
        return true;
    }

    const RenderPlan* m_plan{nullptr};
    FILE* m_pipe{nullptr};
    std::filesystem::path m_temp_video_path;
    renderer2d::detail::TransferCurve m_source_transfer{renderer2d::detail::TransferCurve::sRGB};
    renderer2d::detail::ColorSpace m_source_space{renderer2d::detail::ColorSpace::sRGB};
    renderer2d::detail::TransferCurve m_output_transfer{renderer2d::detail::TransferCurve::sRGB};
    renderer2d::detail::ColorSpace m_output_space{renderer2d::detail::ColorSpace::sRGB};
    renderer2d::detail::ColorRange m_output_range{renderer2d::detail::ColorRange::Full};
    bool m_needs_audio_mux{false};
    std::string m_last_error;
};

} // namespace

std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink() {
    return std::make_unique<FfmpegPipeSink>();
}

} // namespace tachyon::output
