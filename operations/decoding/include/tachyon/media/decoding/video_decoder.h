#pragma once

#include "tachyon/core/media/decoding/video_decoder_interface.h"
#include <memory>
#include <filesystem>
#include <optional>

namespace tachyon::media {

// Legacy adapter for VideoDecoder
class VideoDecoder {
public:
    VideoDecoder() = default;
    ~VideoDecoder() = default;

    bool open(const std::filesystem::path& path);
    void close();

    std::optional<renderer2d::SurfaceRGBA> get_frame_at_time(double seconds);
    bool get_frame_into(double seconds, renderer2d::SurfaceRGBA& target);
    
    [[nodiscard]] double duration() const noexcept;
    [[nodiscard]] double frame_rate() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

private:
    std::unique_ptr<::tachyon::core::media::IVideoDecoder> m_impl;
};

} // namespace tachyon::media
