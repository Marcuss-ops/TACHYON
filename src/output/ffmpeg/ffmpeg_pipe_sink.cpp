#include "tachyon/output/frame_output_sink.h"
#include "ffmpeg_internal.h"
#include "tachyon/runtime/profiling/render_profiler.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "tachyon/core/platform/pipe_process.h"
#include "tachyon/core/platform/process.h"

namespace tachyon::output {

using tachyon::core::platform::open_write_pipe;
using tachyon::core::platform::close_pipe;
using tachyon::core::platform::ProcessSpec;
using tachyon::core::platform::run_process;

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

        bool needs_temp_video = plan.output.profile.format == OutputFormat::Gif || !m_audio_path.empty();

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

        profiling::ProfileScope total_scope(m_profiler, profiling::ProfileEventType::Encode, "ffmpeg_write_frame_total", packet.frame_number);
        
        std::vector<unsigned char> bytes;
        {
            profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::Encode, "color_convert_rgba_to_output", packet.frame_number);
            bytes = convert_and_pack_ffmpeg_frame(
                *packet.frame,
                static_cast<uint32_t>(m_plan->composition.width),
                static_cast<uint32_t>(m_plan->composition.height),
                m_source_transfer, m_source_space,
                m_output_transfer, m_output_space,
                m_output_range);
        }

        std::size_t written = 0;
        {
            profiling::ProfileScope scope(m_profiler, profiling::ProfileEventType::PipeWrite, "ffmpeg_pipe_write", packet.frame_number);
            written = std::fwrite(bytes.data(), 1, bytes.size(), m_pipe);
        }
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

    void set_audio_source(const std::string& audio_path) override {
        m_audio_path = audio_path;
    }

    void set_profiler(profiling::RenderProfiler* profiler) override {
        m_profiler = profiler;
    }

    const std::string& last_error() const override { return m_last_error; }

private:
    bool finalize_post_processing() {
        if (m_plan == nullptr) return true;
        if (m_plan->output.profile.format == OutputFormat::Gif) return finalize_gif();
        if (!m_audio_path.empty()) return finalize_audio();
        return true;
    }

    bool finalize_audio() {
        if (m_temp_video_path.empty()) return true;

        std::string command = build_ffmpeg_mux_command(*m_plan, m_temp_video_path, m_audio_path);
        
        ProcessSpec spec;
#ifdef _WIN32
        spec.executable = "cmd.exe";
        spec.args = {"/C", command};
#else
        spec.executable = "sh";
        spec.args = {"-c", command};
#endif
        auto result = run_process(spec);
        if (!result.success) {
            m_last_error = "ffmpeg muxing failed: " + result.error;
            return false;
        }

        std::error_code ec;
        std::filesystem::remove(m_temp_video_path, ec);
        return true;
    }

    bool finalize_gif() {
        const std::filesystem::path destination(m_plan->output.destination.path);
        const std::filesystem::path palette_path = make_ffmpeg_temp_path(destination, "palette", ".png");

        // Run palettegen
        ProcessSpec spec1;
        spec1.executable = "ffmpeg";
        spec1.args = {
            "-hide_banner",
            "-loglevel", "error",
            "-y",
            "-i", m_temp_video_path.string(),
            "-vf", "palettegen",
            palette_path.string()
        };
        auto r1 = run_process(spec1);
        if (!r1.success) {
            m_last_error = "ffmpeg palettegen failed"
                + std::string("\nstdout:\n") + r1.output
                + std::string("\nstderr:\n") + r1.error;
            return false;
        }

        // Run paletteuse
        ProcessSpec spec2;
        spec2.executable = "ffmpeg";
        spec2.args = {
            "-hide_banner",
            "-loglevel", "error",
            "-y",
            "-i", m_temp_video_path.string(),
            "-i", palette_path.string(),
            "-lavfi", "paletteuse",
            destination.string()
        };
        auto r2 = run_process(spec2);
        if (!r2.success) {
            m_last_error = "ffmpeg paletteuse failed"
                + std::string("\nstdout:\n") + r2.output
                + std::string("\nstderr:\n") + r2.error;
            return false;
        }

        std::filesystem::remove(m_temp_video_path);
        std::filesystem::remove(palette_path);
        return true;
    }



    const RenderPlan* m_plan{nullptr};
    FILE* m_pipe{nullptr};
    std::filesystem::path m_temp_video_path;
    std::string m_audio_path;
    renderer2d::detail::TransferCurve m_source_transfer{renderer2d::detail::TransferCurve::sRGB};
    renderer2d::detail::ColorSpace m_source_space{renderer2d::detail::ColorSpace::sRGB};
    renderer2d::detail::TransferCurve m_output_transfer{renderer2d::detail::TransferCurve::sRGB};
    renderer2d::detail::ColorSpace m_output_space{renderer2d::detail::ColorSpace::sRGB};
    renderer2d::detail::ColorRange m_output_range{renderer2d::detail::ColorRange::Full};
    std::string m_last_error;
    profiling::RenderProfiler* m_profiler{nullptr};
};

std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink() {
    return std::make_unique<FfmpegPipeSink>();
}

} // namespace tachyon::output
