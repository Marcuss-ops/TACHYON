#include "tachyon/renderer2d/resource/resource_provider.h"
#include "tachyon/core/media/media_provider.h"
#include <algorithm>

namespace tachyon::renderer2d {

std::shared_ptr<std::uint8_t[]> RendererResourceProvider::get_texture_rgba(const std::string& id) {
#ifdef TACHYON_ENABLE_MEDIA
    if (!m_context.media) return nullptr;
    
    auto img = m_context.media->get_image(id);
    if (!img) {
        return nullptr;
    }
    
    size_t width = img->width();
    size_t height = img->height();
    size_t size = width * height * 4;
    auto data = std::make_shared<std::uint8_t[]>(size);
    
    const auto& pixels = img->pixels();
    for (size_t i = 0; i < width * height; ++i) {
        // Simple float to 8-bit conversion (assuming 0-1 range)
        data[i * 4 + 0] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 0] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 1] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 1] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 2] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 2] * 255.0f, 0.0f, 255.0f));
        data[i * 4 + 3] = static_cast<std::uint8_t>(std::clamp(pixels[i * 4 + 3] * 255.0f, 0.0f, 255.0f));
    }
    
    return data;
#else
    (void)id;
    return nullptr;
#endif
}

} // namespace tachyon::renderer2d
