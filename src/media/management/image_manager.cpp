#include "tachyon/media/management/image_manager.h"
#include "tachyon/renderer2d/color/color_transfer.h"

#include <stb_image.h>

namespace tachyon::media {

namespace {

constexpr std::uint32_t kFallbackSurfaceWidth = 256U;
constexpr std::uint32_t kFallbackSurfaceHeight = 256U;
constexpr std::uint32_t kFallbackCheckerSize = 16U;
constexpr renderer2d::Color kFallbackCheckerLight{1.0f, 0.0f, 1.0f, 1.0f};
constexpr renderer2d::Color kFallbackCheckerDark{0.25f, 0.0f, 0.25f, 1.0f};
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

static std::unique_ptr<renderer2d::SurfaceRGBA> decode_image(const std::filesystem::path& path, AlphaMode alpha_mode) {
    int w, h, channels;
    unsigned char* data = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
    if (!data) return nullptr;

    const uint32_t uw = static_cast<uint32_t>(w);
    const uint32_t uh = static_cast<uint32_t>(h);
    auto surface = std::make_unique<renderer2d::SurfaceRGBA>(uw, uh);
    
    // Direct access to pixels if possible, but SurfaceRGBA doesn't expose a mutable ref to the whole vector
    // So we use a faster loop
    for (uint32_t i = 0; i < uw * uh; ++i) {
        const float r = renderer2d::detail::sRGB_to_Linear_f(static_cast<float>(data[i * 4 + 0]) / 255.0f);
        const float g = renderer2d::detail::sRGB_to_Linear_f(static_cast<float>(data[i * 4 + 1]) / 255.0f);
        const float b = renderer2d::detail::sRGB_to_Linear_f(static_cast<float>(data[i * 4 + 2]) / 255.0f);
        float a = static_cast<float>(data[i * 4 + 3]) / 255.0f;

        if (alpha_mode == AlphaMode::Ignore) {
            a = 1.0f;
        }

        // Convert sRGB (from STB) to Linear
        renderer2d::Color linear_color{r, g, b, a};

        if (alpha_mode == AlphaMode::Premultiplied) {
            linear_color.r *= a;
            linear_color.g *= a;
            linear_color.b *= a;
        }

        surface->set_pixel(i % uw, i / uw, linear_color);
    }

    stbi_image_free(data);
    return surface;
}

static std::unique_ptr<HDRTextureData> decode_hdr_image(const std::filesystem::path& path) {
    int w, h, channels;
    // Environment maps are usually RGB, using 3 channels to save memory
    float* data = stbi_loadf(path.string().c_str(), &w, &h, &channels, 3);
    if (!data) return nullptr;

    auto hdr = std::make_unique<HDRTextureData>();
    hdr->width = w;
    hdr->height = h;
    hdr->channels = 3;
    hdr->data.assign(data, data + (w * h * 3));

    stbi_image_free(data);
    return hdr;
}

const renderer2d::SurfaceRGBA* ImageManager::get_image(const std::filesystem::path& path, AlphaMode alpha_mode, DiagnosticBag* diagnostics) {
    std::string key = path.string();
    if (alpha_mode == AlphaMode::Premultiplied) key += ":premultiplied";
    else if (alpha_mode == AlphaMode::Ignore) key += ":ignore";

    { // lookup cached image
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) return it->second.get();
    }

    // decode outside the lock (expensive)
    auto surface = decode_image(path, alpha_mode);
    if (!surface) {
        surface = make_fallback_surface();
        std::lock_guard<std::mutex> lock(m_mutex);
        record_missing_image(m_diagnostics, key, kDecodeFailedCode, (std::string(kDecodeFailedMessage) + ": " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown")).c_str());
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

const HDRTextureData* ImageManager::get_hdr_image(const std::filesystem::path& path, DiagnosticBag* diagnostics) {
    const std::string key = path.string();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_hdr_cache.find(key);
        if (it != m_hdr_cache.end()) return it->second.get();
    }

    auto hdr = decode_hdr_image(path);
    if (!hdr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        record_missing_image(m_diagnostics, key, kDecodeFailedCode, (std::string(kDecodeFailedMessage) + " (HDR): " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown")).c_str());
        if (diagnostics) {
            record_missing_image(*diagnostics, key, kDecodeFailedCode, kDecodeFailedMessage);
        }
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hdr_cache.find(key);
    if (it != m_hdr_cache.end()) return it->second.get();

    const HDRTextureData* ptr = hdr.get();
    m_hdr_cache[key] = std::move(hdr);
    return ptr;
}

DiagnosticBag ImageManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    DiagnosticBag diagnostics;
    diagnostics.diagnostics = std::move(m_diagnostics.diagnostics);
    return diagnostics;
}

void ImageManager::store_image(const std::string& key, std::unique_ptr<renderer2d::SurfaceRGBA> image) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[key] = std::move(image);
}

void ImageManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_hdr_cache.clear();
    m_diagnostics.diagnostics.clear();
}

} // namespace tachyon::media
