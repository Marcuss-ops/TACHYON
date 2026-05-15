#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <filesystem>
#include <optional>

namespace tachyon::core::media {

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool open(const std::filesystem::path& path) = 0;
    virtual void close() = 0;

    virtual std::optional<renderer2d::SurfaceRGBA> get_frame_at_time(double seconds) = 0;
    virtual bool get_frame_into(double seconds, renderer2d::SurfaceRGBA& target) = 0;
    
    virtual double duration() const noexcept = 0;
    virtual double frame_rate() const noexcept = 0;
    virtual int width() const noexcept = 0;
    virtual int height() const noexcept = 0;
};

} // namespace tachyon::core::media
