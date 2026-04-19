#include "tachyon/media/image_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace tachyon::media {

namespace {

constexpr std::uint32_t kFallbackSurfaceWidth = 256U;
constexpr std::uint32_t kFallbackSurfaceHeight = 256U;
constexpr std::uint32_t kFallbackCheckerSize = 16U;
constexpr renderer2d::Color kFallbackCheckerLight{255, 0, 255, 255};
constexpr renderer2d::Color kFallbackCheckerDark{64, 0, 64, 255};
constexpr const char* kDecodeFailedCode = "media.image.decode_failed";
constexpr const char* kDecodeFailedMessage = "failed to decode image, using fallback surface";

std::unique_ptr<renderer2d::SurfaceRGBA> make_fallback_surface() {
    auto surface = std::make_unique<renderer2d::SurfaceRGBA>(kFallbackSurfaceWidth, kFallbackSurfaceHeight);
    for (std::uint32_t y = 0; y < kFallbackSurfaceHeight; ++y) {
        for (std::uint32_t x = 0; x < kFallbackSurfaceWidth; ++x) {
            const bool checker = ((x / kFallbackCheckerSize) + (y / kFallbackCheckerSize)) % 2U == 0U;
            const renderer2d::Color color = checker ? kFallbackCheckerLight : kFallbackCheckerDark;
            surface->set_pixel(x, y, color);
        }
    }
    return surface;
}

void record_missing_image(DiagnosticBag& diagnostics, const std::string& key, const char* code, const char* message) {
    diagnostics.add_warning(code, message, key);
}

} // namespace

static std::unique_ptr<renderer2d::SurfaceRGBA> decode_image(const std::filesystem::path& path) {
    int w, h, channels;
    unsigned char* data = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
    if (!data) return nullptr;

    auto surface = std::make_unique<renderer2d::SurfaceRGBA>(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * 4;
            renderer2d::Color c{data[i], data[i + 1], data[i + 2], data[i + 3]};
            surface->set_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), c);
        }
    }
    stbi_image_free(data);
    return surface;
}

const renderer2d::SurfaceRGBA* ImageManager::get_image(const std::filesystem::path& path, DiagnosticBag* diagnostics) {
    const std::string key = path.string();

    { // lookup cached image
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) return it->second.get();
    }

    // decode outside the lock (expensive)
    auto surface = decode_image(path);
    if (!surface) {
        surface = make_fallback_surface();
        std::lock_guard<std::mutex> lock(m_mutex);
        record_missing_image(m_diagnostics, key, kDecodeFailedCode, kDecodeFailedMessage);
        if (diagnostics) {
            record_missing_image(*diagnostics, key, kDecodeFailedCode, kDecodeFailedMessage);
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    // double-check: another thread might have loaded it in the meantime
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second.get();

    const renderer2d::SurfaceRGBA* ptr = surface.get();
    m_cache[key] = std::move(surface);
    return ptr;
}

DiagnosticBag ImageManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    DiagnosticBag diagnostics;
    diagnostics.diagnostics = std::move(m_diagnostics.diagnostics);
    return diagnostics;
}

void ImageManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_diagnostics.diagnostics.clear();
}

} // namespace tachyon::media
