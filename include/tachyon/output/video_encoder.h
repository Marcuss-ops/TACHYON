#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::output {

struct VideoEncoderOptions {
    std::uint32_t width{0};
    std::uint32_t height{0};
    double fps{60.0};
    std::string codec;
    std::string pixel_format;
    bool alpha{false};
    std::string color_space{"srgb"};
    std::string color_transfer{"srgb"};
    std::string color_range{"full"};
};

class VideoEncoderBackend {
public:
    virtual ~VideoEncoderBackend() = default;

    virtual bool initialize(const std::string& output_path, const VideoEncoderOptions& options) = 0;
    virtual bool push_frame(const renderer2d::Framebuffer& frame) = 0;
    virtual bool finalize() = 0;

    [[nodiscard]] virtual const std::string& last_error() const = 0;
};

} // namespace tachyon::output
