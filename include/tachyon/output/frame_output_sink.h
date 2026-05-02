#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/output/frame_aov.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tachyon::output {

struct FrameMetadata {
    double time_seconds{0.0};
    std::string timecode;
    std::string color_space{"linear_rec709"};
    std::string scene_hash;
};

struct OutputFramePacket {
    std::int64_t frame_number{0};
    const renderer2d::Framebuffer* frame{nullptr};
    std::vector<FrameAOV> aovs;   // optional: multi-layer AOVs for EXR output
    FrameMetadata metadata;
};

class FrameOutputSink {
    public:
        virtual ~FrameOutputSink() = default;

        virtual bool begin(const RenderPlan& plan) = 0;
        virtual bool write_frame(const OutputFramePacket& packet) = 0;
        virtual bool finish() = 0;
        virtual bool finalize_post_processing() { return true; }
        virtual void set_audio_source(const std::string& audio_path) { (void)audio_path; }
        [[nodiscard]] virtual const std::string& last_error() const = 0;
};

std::unique_ptr<FrameOutputSink> create_png_sequence_sink();
std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink();
std::unique_ptr<FrameOutputSink> create_exr_sequence_sink();  // requires TACHYON_EXR
std::unique_ptr<FrameOutputSink> create_frame_output_sink(const RenderPlan& plan);

bool output_requests_png_sequence(const OutputContract& contract);
bool output_requests_video_file(const OutputContract& contract);
bool output_requests_exr(const OutputContract& contract);

} // namespace tachyon::output
