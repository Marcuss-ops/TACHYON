#include "tachyon/media/import/asset_import_pipeline.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <filesystem>

namespace tachyon::media {

AssetImportPipeline::AssetImportPipeline(MediaManager& media_manager)
    : m_media_manager(media_manager) {}

ImportResult AssetImportPipeline::import(const ImportRequest& request) {
    ImportResult result;

    // Step 1: Resolve path (original vs proxy)
    std::filesystem::path resolved = m_media_manager.resolve_media_path(
        request.path, request.purpose);
    result.resolved_path = resolved;

    if (resolved.empty() || !std::filesystem::exists(resolved)) {
        result.status = ImportResult::NotFound;
        result.error_message = "Asset not found: " + request.path.string();
        return result;
    }

    // Step 2: Import based on type
    switch (request.type) {
        case AssetType::IMAGE:
            return import_image(request, resolved);
        case AssetType::VIDEO:
            return import_video(request, resolved);
        case AssetType::MESH:
            return import_mesh(request, resolved);
        case AssetType::AUDIO:
            // Audio handled separately by AudioMixer
            result.status = ImportResult::DecodeError;
            result.error_message = "Audio import not supported through this pipeline";
            return result;
        default:
            result.status = ImportResult::DecodeError;
            result.error_message = "Unknown asset type";
            return result;
    }
}

ImportResult AssetImportPipeline::import_image(const ImportRequest& request,
                                               const std::filesystem::path& resolved) {
    ImportResult result;
    result.resolved_path = resolved;

    DiagnosticBag diagnostics;
    const renderer2d::SurfaceRGBA* surface = m_media_manager.get_image(
        resolved, request.alpha_mode, &diagnostics);

    if (!surface) {
        result.status = ImportResult::DecodeError;
        result.error_message = "Failed to decode image: " + resolved.string();
        return result;
    }

    result.surface = surface;
    result.status = ImportResult::Success;
    return result;
}

ImportResult AssetImportPipeline::import_video(const ImportRequest& request,
                                               const std::filesystem::path& resolved) {
    ImportResult result;
    result.resolved_path = resolved;

    DiagnosticBag diagnostics;
    const renderer2d::SurfaceRGBA* surface = m_media_manager.get_video_frame(
        resolved, request.time_seconds, &diagnostics);

    if (!surface) {
        result.status = ImportResult::DecodeError;
        result.error_message = "Failed to decode video frame: " + resolved.string();
        return result;
    }

    result.surface = surface;
    result.status = ImportResult::Success;
    return result;
}

ImportResult AssetImportPipeline::import_mesh(const ImportRequest& request,
                                              const std::filesystem::path& resolved) {
    ImportResult result;
    result.resolved_path = resolved;

    DiagnosticBag diagnostics;
    auto mesh = m_media_manager.get_mesh(resolved, &diagnostics);

    if (!mesh) {
        result.status = ImportResult::DecodeError;
        result.error_message = "Failed to load mesh: " + resolved.string();
        return result;
    }

    result.mesh = mesh;
    result.status = ImportResult::Success;
    return result;
}

} // namespace tachyon::media
