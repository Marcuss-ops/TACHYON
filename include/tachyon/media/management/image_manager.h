#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/media/loading/mesh_asset.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include <mutex>

namespace tachyon::media {

enum class AlphaMode {
    Straight,
    Premultiplied,
    Ignore
};

class ImageManager {
public:
    ImageManager() = default;

    /**
     * Get or load an image from the given path.
     * Returns a pointer to the surface. On failure, a fallback surface is returned
     * and diagnostics are recorded.
     */
    const renderer2d::SurfaceRGBA* get_image(
        const std::filesystem::path& path, 
        AlphaMode alpha_mode = AlphaMode::Straight,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * Get or load an HDR image (environment map).
     */
    const HDRTextureData* get_hdr_image(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * Consume and clear accumulated media diagnostics.
     */
    DiagnosticBag consume_diagnostics();

    /**
     * Store an image manually in the cache.
     */
    void store_image(const std::string& key, std::unique_ptr<renderer2d::SurfaceRGBA> image);

    /**
     * Clear the cache.
     */
    void clear_cache();

private:
    std::map<std::string, std::unique_ptr<renderer2d::SurfaceRGBA>> m_cache;
    std::map<std::string, std::unique_ptr<HDRTextureData>> m_hdr_cache;
    DiagnosticBag m_diagnostics;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
