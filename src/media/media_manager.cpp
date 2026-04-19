#include "tachyon/media/media_manager.h"

namespace tachyon::media {

const renderer2d::SurfaceRGBA* MediaManager::get_image(const std::filesystem::path& path, DiagnosticBag* diagnostics) {
    return m_image_manager.get_image(path, diagnostics);
}

VideoDecoder* MediaManager::get_video_decoder(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::string key = path.string();
    auto it = m_video_cache.find(key);
    if (it != m_video_cache.end()) {
        return it->second.get();
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->open(path)) {
        return nullptr;
    }

    auto* result = decoder.get();
    m_video_cache.emplace(key, std::move(decoder));
    return result;
}

void MediaManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_image_manager.clear_cache();
    m_video_cache.clear();
}

DiagnosticBag MediaManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_image_manager.consume_diagnostics();
}

} // namespace tachyon::media
