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
    
    // Encoding quality settings
    int crf{23};                        // 0-51, 18=visually lossless, 23=default (H.264/VP9)
    int bitrate_kbps{0};                // 0 = usa CRF, >0 = bitrate target
    std::string preset{"medium"};       // ultrafast/fast/medium/slow/veryslow
    int audio_bitrate_kbps{192};        // Audio bitrate in kbps
    bool faststart{true};               // mp4 moov atom in testa (streaming-friendly)
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
