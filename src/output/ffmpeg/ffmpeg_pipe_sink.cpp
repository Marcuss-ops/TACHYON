#include "tachyon/output/frame_output_sink.h"
#include "tachyon/audio/core/audio_mixer.h"
#include "tachyon/audio/io/audio_decoder.h"
#include "ffmpeg_internal.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#if defined(_WIN32)
#define TACHYON_POPEN _popen
#define TACHYON_PCLOSE _pclose
#else
#define TACHYON_POPEN popen
#define TACHYON_PCLOSE pclose
#endif

namespace tachyon::output {

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

        m_needs_audio_mux = !plan.output.profile.audio.mode.empty() &&
                            plan.output.profile.audio.mode != "none" &&
                            !plan.output.profile.audio.tracks.empty();
        
        bool needs_temp_video = m_needs_audio_mux || plan.output.profile.format == OutputFormat::Gif;

        const std::filesystem::path destination(plan.output.destination.path);
        if (!destination.parent_path().empty()) {
            std::filesystem::create_directories(destination.parent_path());
        }

        if (needs_temp_video) {
            m_temp_video_path = make_ffmpeg_temp_path(destination, "video", ".mkv");
        }

        const std::string command = needs_temp_video
            ? build_ffmpeg_video_command(plan, m_temp_video_path, false)
            : build_ffmpeg_video_command(plan, destination, true);

        m_pipe = TACHYON_POPEN(command.c_str(), "wb");
        if (m_pipe == nullptr) {
            m_last_error = "failed to open ffmpeg pipe";
            return false;
        }

        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        m_last_error.clear();
        if (m_pipe == nullptr) return false;
        if (packet.frame == nullptr) return false;

        const std::vector<unsigned char> bytes = convert_and_pack_ffmpeg_frame(
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
        if (m_pipe != nullptr) {
            const int status = TACHYON_PCLOSE(m_pipe);
            m_pipe = nullptr;
            if (status != 0) {
                m_last_error = "ffmpeg exited with a non-zero status";
                return false;
            }
        }

        return finalize_post_processing();
    }

    const std::string& last_error() const override { return m_last_error; }

private:
    bool finalize_post_processing() {
        if (m_plan == nullptr) return true;
        if (m_plan->output.profile.format == OutputFormat::Gif) return finalize_gif();
        if (m_needs_audio_mux) return finalize_audio_mux();
        return true;
    }

    bool finalize_gif() {
        const std::filesystem::path destination(m_plan->output.destination.path);
        const std::filesystem::path palette_path = make_ffmpeg_temp_path(destination, "palette", ".png");
        
        // Simplified GIF finalization (could be further modularized)
        std::string p1_cmd = "ffmpeg -y -i \"" + m_temp_video_path.string() + "\" -vf \"palettegen\" \"" + palette_path.string() + "\"";
        if (std::system(p1_cmd.c_str()) != 0) return false;

        std::string p2_cmd = "ffmpeg -y -i \"" + m_temp_video_path.string() + "\" -i \"" + palette_path.string() + "\" -lavfi \"paletteuse\" \"" + destination.string() + "\"";
        if (std::system(p2_cmd.c_str()) != 0) return false;

        std::filesystem::remove(m_temp_video_path);
        std::filesystem::remove(palette_path);
        return true;
    }

    bool finalize_audio_mux() {
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

        std::string audio_cmd = "ffmpeg -y -f f32le -ar 48000 -ac 2 -i - \"" + master_audio_path.string() + "\"";
        FILE* audio_pipe = TACHYON_POPEN(audio_cmd.c_str(), "wb");
        if (!audio_pipe) return false;

        const double duration = m_plan->composition.duration;
        std::vector<float> mix_buffer;
        for (double t = 0.0; t < duration; t += 1.0) {
            mixer.mix(t, std::min(1.0, duration - t), mix_buffer);
            std::fwrite(mix_buffer.data(), sizeof(float), mix_buffer.size(), audio_pipe);
        }
        TACHYON_PCLOSE(audio_pipe);

        const std::string mux_command = build_ffmpeg_mux_command(*m_plan, m_temp_video_path, master_audio_path);
        if (std::system(mux_command.c_str()) != 0) return false;

        std::filesystem::remove(m_temp_video_path);
        std::filesystem::remove(master_audio_path);
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

std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink() {
    return std::make_unique<FfmpegPipeSink>();
}

} // namespace tachyon::output

