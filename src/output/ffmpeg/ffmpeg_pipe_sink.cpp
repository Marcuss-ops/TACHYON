#include "tachyon/output/frame_output_sink.h"
#include "tachyon/audio/audio_mixer.h"
#include "tachyon/audio/audio_decoder.h"
#include "ffmpeg_internal.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "tachyon/core/platform/pipe_process.h"
#include "tachyon/core/platform/process.h"

namespace tachyon::output {

using tachyon::core::platform::open_write_pipe;
using tachyon::core::platform::close_pipe;

class FfmpegPipeSink final : public FrameOutputSink {
public:
    ~FfmpegPipeSink() override {
        if (m_pipe != nullptr) {
            close_pipe(m_pipe);
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
                            audio::has_any_audio(plan);
        
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

        m_pipe = open_write_pipe(command.c_str());
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
            const int status = close_pipe(m_pipe);
            m_pipe = nullptr;
            if (status != 0) {
                m_last_error = "ffmpeg exited with a non-zero status";
                return false;
            }
        }

        return finalize_post_processing();
    }

    const std::string& last_error() const override { return m_last_error; }

    static bool run_shell_cmd(const std::string& cmd) {
        using namespace tachyon::core::platform;
        ProcessSpec spec;
#ifdef _WIN32
        spec.executable = "cmd.exe";
        spec.args = {"/C", cmd};
#else
        spec.executable = "sh";
        spec.args = {"-c", cmd};
#endif
        auto r = run_process(spec);
        return r.success && r.exit_code == 0;
    }

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
        if (!run_shell_cmd(p1_cmd)) return false;

        std::string p2_cmd = "ffmpeg -y -i \"" + m_temp_video_path.string() + "\" -i \"" + palette_path.string() + "\" -lavfi \"paletteuse\" \"" + destination.string() + "\"";
        if (!run_shell_cmd(p2_cmd)) return false;

        std::filesystem::remove(m_temp_video_path);
        std::filesystem::remove(palette_path);
        return true;
    }

    bool finalize_audio_mux() {
        const std::filesystem::path destination(m_plan->output.destination.path);
        const std::filesystem::path master_audio_path = destination.parent_path() / (destination.stem().string() + ".tachyon.master_audio.wav");

        if (!audio::export_plan_audio(*m_plan, master_audio_path)) {
            return false;
        }

        const std::string mux_command = build_ffmpeg_mux_command(*m_plan, m_temp_video_path, master_audio_path);
        if (!run_shell_cmd(mux_command)) return false;

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
