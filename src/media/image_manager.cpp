#include "tachyon/media/image_manager.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace tachyon::media {

static std::unique_ptr<renderer2d::SurfaceRGBA> decode_image(const std::filesystem::path& path) {
    int w, h, channels;
    unsigned char* data = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
    if (!data) return nullptr;

    auto surface = std::make_unique<renderer2d::SurfaceRGBA>(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * 4;
            renderer2d::Color c{data[i], data[i+1], data[i+2], data[i+3]};
            // premoltiplica subito — il resto del motore se lo aspetta
            c.r = static_cast<uint8_t>((uint32_t)c.r * c.a / 255U);
            c.g = static_cast<uint8_t>((uint32_t)c.g * c.a / 255U);
            c.b = static_cast<uint8_t>((uint32_t)c.b * c.a / 255U);
            surface->set_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), c);
        }
    }
    stbi_image_free(data);
    return surface;
}

ImageManager& ImageManager::instance() {
    static ImageManager inst;
    return inst;
}

const renderer2d::SurfaceRGBA* ImageManager::get_image(const std::filesystem::path& path) {
    const std::string key = path.string();

    { // lookup cached image
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) return it->second.get();
    }

    // decode outside the lock (expensive)
    auto surface = decode_image(path);

    std::lock_guard<std::mutex> lock(m_mutex);
    // double-check: another thread might have loaded it in the meantime
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second.get();

    if (!surface) {
        // Fallback: Generate a diagnostic pattern if decoding failed
        surface = std::make_unique<renderer2d::SurfaceRGBA>(256, 256);
        for (uint32_t y = 0; y < 256; ++y) {
            for (uint32_t x = 0; x < 256; ++x) {
                uint8_t r = static_cast<uint8_t>(x);
                uint8_t g = static_cast<uint8_t>(y);
                uint8_t b = 128;
                if ((x / 16 + y / 16) % 2 == 0) {
                    surface->set_pixel(x, y, renderer2d::Color{r, g, b, 255});
                } else {
                    surface->set_pixel(x, y, renderer2d::Color{64, 64, 64, 255});
                }
            }
        }
        std::cout << "[ImageManager] Generated fallback pattern for missing/corrupt image: " << key << std::endl;
    }

    const renderer2d::SurfaceRGBA* ptr = surface.get();
    m_cache[key] = std::move(surface);
    return ptr;
}

void ImageManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

} // namespace tachyon::media
