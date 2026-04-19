#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include <mutex>

namespace tachyon::media {

class ImageManager {
public:
    static ImageManager& instance();

    /**
     * Get or load an image from the given path.
     * Returns a pointer to the surface, or nullptr if it fails to load.
     */
    const renderer2d::SurfaceRGBA* get_image(const std::filesystem::path& path);

    /**
     * Clear the cache.
     */
    void clear_cache();

private:
    ImageManager() = default;
    ~ImageManager() = default;

    std::map<std::string, std::unique_ptr<renderer2d::SurfaceRGBA>> m_cache;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
