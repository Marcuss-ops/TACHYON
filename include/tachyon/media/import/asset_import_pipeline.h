#pragma once

#include "tachyon/media/asset_manager.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/media/loading/mesh_asset.h"
#include <filesystem>
#include <string>

namespace tachyon::media {

struct ImportRequest {
    std::filesystem::path path;
    AssetType type{AssetType::IMAGE};
    ResolutionPurpose purpose{ResolutionPurpose::Playback};
    AlphaMode alpha_mode{AlphaMode::Straight};
    double time_seconds{0.0};  // For video frames
    bool force_reload{false};
};

struct ImportResult {
    enum Status {
        Success,
        NotFound,
        DecodeError,
        ProxyPending
    };

    Status status{Success};
    const renderer2d::SurfaceRGBA* surface{nullptr};
    const HDRTextureData* hdr_surface{nullptr};
    std::shared_ptr<const MeshAsset> mesh{nullptr};
    std::filesystem::path resolved_path;
    std::string error_message;
};

class AssetImportPipeline {
public:
    explicit AssetImportPipeline(MediaManager& media_manager);

    ImportResult import(const ImportRequest& request);

private:
    MediaManager& m_media_manager;

    ImportResult import_image(const ImportRequest& request,
                              const std::filesystem::path& resolved);
    ImportResult import_video(const ImportRequest& request,
                              const std::filesystem::path& resolved);
    ImportResult import_mesh(const ImportRequest& request,
                             const std::filesystem::path& resolved);
};

} // namespace tachyon::media

