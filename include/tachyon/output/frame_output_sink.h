#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/render_plan.h"

#include <cstdint>
#include <memory>
#include <string>

namespace tachyon::output {

struct OutputFramePacket {
    std::int64_t frame_number{0};
    const renderer2d::Framebuffer* frame{nullptr};
};

class FrameOutputSink {
public:
    virtual ~FrameOutputSink() = default;

    virtual bool begin(const RenderPlan& plan) = 0;
    virtual bool write_frame(const OutputFramePacket& packet) = 0;
    virtual bool finish() = 0;
    [[nodiscard]] virtual const std::string& last_error() const = 0;
};

std::unique_ptr<FrameOutputSink> create_png_sequence_sink();
std::unique_ptr<FrameOutputSink> create_ffmpeg_pipe_sink();
std::unique_ptr<FrameOutputSink> create_frame_output_sink(const RenderPlan& plan);

bool output_requests_png_sequence(const OutputContract& contract);
bool output_requests_video_file(const OutputContract& contract);

} // namespace tachyon::output
