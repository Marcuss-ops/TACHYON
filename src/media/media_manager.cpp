#include "tachyon/media/media_manager.h"

#include "tachyon/media/image_manager.h"

namespace tachyon::media {

MediaManager& MediaManager::instance() {
    static MediaManager inst;
    return inst;
}

const renderer2d::SurfaceRGBA* MediaManager::get_image(const std::filesystem::path& path) {
    return ImageManager::instance().get_image(path);
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
    m_image_cache.clear();
    m_video_cache.clear();
    ImageManager::instance().clear_cache();
}

} // namespace tachyon::media
