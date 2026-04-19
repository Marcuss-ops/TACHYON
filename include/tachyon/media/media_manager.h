#pragma once

#include "tachyon/media/image_manager.h"
#include "tachyon/media/video_decoder.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/diagnostics.h"

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

namespace tachyon::media {

class MediaManager {
public:
    MediaManager() = default;

    const renderer2d::SurfaceRGBA* get_image(const std::filesystem::path& path, DiagnosticBag* diagnostics = nullptr);
    VideoDecoder* get_video_decoder(const std::filesystem::path& path);
    DiagnosticBag consume_diagnostics();
    void clear_cache();

private:
    ImageManager m_image_manager;
    std::map<std::string, std::unique_ptr<VideoDecoder>> m_video_cache;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
