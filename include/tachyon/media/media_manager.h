#pragma once

#include "tachyon/media/image_manager.h"
#include "tachyon/media/mesh_asset.h"
#include "tachyon/media/video_decoder.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/core/diagnostics.h"

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

namespace tachyon::media {

class MediaManager {
public:
    MediaManager() = default;

    const renderer2d::SurfaceRGBA* get_image(
        const std::filesystem::path& path, 
        AlphaMode alpha_mode = AlphaMode::Straight,
        DiagnosticBag* diagnostics = nullptr);

    const HDRTextureData* get_hdr_image(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);

    /**
     * Acquires a VideoDecoder for the given path from a pool.
     * Must be returned via release_video_decoder.
     */
    VideoDecoder* acquire_video_decoder(const std::filesystem::path& path);
    void release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder);

    const MeshAsset* get_mesh(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);

    DiagnosticBag consume_diagnostics();
    void clear_cache();

private:
    struct VideoPool {
        std::vector<std::unique_ptr<VideoDecoder>> available;
        std::mutex mutex;
    };

    ImageManager m_image_manager;
    std::map<std::string, std::shared_ptr<VideoPool>> m_video_pools;
    std::map<std::string, std::unique_ptr<MeshAsset>> m_mesh_cache;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
