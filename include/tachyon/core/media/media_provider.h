#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/core/render_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <string>
#include <memory>
#include <filesystem>

namespace tachyon::media {

/**
 * @brief Interface for resolving and providing media surfaces to the renderer.
 */
class IMediaProvider {
public:
    virtual ~IMediaProvider() = default;

    /**
     * @brief Gets an image surface for the given path/id.
     */
    virtual const renderer2d::SurfaceRGBA* get_image(
        const std::filesystem::path& path,
        ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight,
        ::tachyon::DiagnosticBag* diagnostics = nullptr) = 0;

    /**
     * @brief Gets a video frame surface for the given path/id at the specified time.
     */
    virtual const renderer2d::SurfaceRGBA* get_video_frame(
        const std::filesystem::path& path,
        double time,
        ::tachyon::DiagnosticBag* diagnostics = nullptr) = 0;

    /**
     * @brief Gets the duration of a media asset in seconds.
     */
    virtual double get_duration(const std::filesystem::path& path) = 0;

    /**
     * @brief Resolves a path using internal manager logic (proxies, etc).
     */
    virtual std::filesystem::path resolve_media_path(const std::filesystem::path& path) const = 0;
};

} // namespace tachyon::media
