#pragma once
#include "tachyon/output/frame_output_sink.h"
#include <memory>

namespace tachyon {
namespace output {

struct ProResSinkConfig {
    std::string profile{"prores_422"};
    int width{1920};
    int height{1080};
    int framerate{30};
    std::string pixel_format{"yuv422p10le"};
};

class ProResSink : public FrameOutputSink {
public:
    explicit ProResSink(const ProResSinkConfig& config);
    ~ProResSink() override;

    bool begin(const RenderPlan& plan) override;
    bool write_frame(const OutputFramePacket& packet) override;
    bool finish() override;
    bool finalize_post_processing() override;
    [[nodiscard]] const std::string& last_error() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::unique_ptr<FrameOutputSink> create_prores_sink(const ProResSinkConfig& config = ProResSinkConfig{});

} // namespace output
} // namespace tachyon
