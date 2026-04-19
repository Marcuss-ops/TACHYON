#pragma once

#include "tachyon/media/video_decoder.h"
#include "tachyon/renderer2d/framebuffer.h"

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

namespace tachyon::media {

class MediaManager {
public:
    static MediaManager& instance();

    const renderer2d::SurfaceRGBA* get_image(const std::filesystem::path& path);
    VideoDecoder* get_video_decoder(const std::filesystem::path& path);
    void clear_cache();

private:
    MediaManager() = default;
    ~MediaManager() = default;

    std::map<std::string, std::unique_ptr<renderer2d::SurfaceRGBA>> m_image_cache;
    std::map<std::string, std::unique_ptr<VideoDecoder>> m_video_cache;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
