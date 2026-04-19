#include "tachyon/media/image_manager.h"

#include <iostream>

// Integration of stb_image for real decoding. 
// If stb_image.h is missing, uncomment the implementation once the header is available.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#if __has_include("stb_image.h")
    #include "stb_image.h"
    #define TACHYON_HAS_STBI
#endif

namespace tachyon::media {

ImageManager& ImageManager::instance() {
    static ImageManager inst;
    return inst;
}

const renderer2d::SurfaceRGBA* ImageManager::get_image(const std::filesystem::path& path) {
    const std::string key = path.string();
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        return it->second.get();
    }

    // Try to load real image (conceptual placeholder for now)
    std::unique_ptr<renderer2d::SurfaceRGBA> surface;

#ifdef TACHYON_HAS_STBI
    // Real implementation using stb_image
    /*
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (data) {
        surface = std::make_unique<renderer2d::SurfaceRGBA>(width, height);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int offset = (y * width + x) * 4;
                surface->set_pixel(x, y, renderer2d::Color{
                    data[offset], data[offset + 1], data[offset + 2], data[offset + 3]
                });
            }
        }
        stbi_image_free(data);
    }
    */
#endif

    if (!surface) {
        // Fallback: Generate a diagnostic pattern
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
        std::cout << "[ImageManager] Generated fallback pattern for: " << key << std::endl;
    }

    m_cache[key] = std::move(surface);
    return m_cache[key].get();
}

void ImageManager::clear_cache() {
    m_cache.clear();
}

} // namespace tachyon::media
