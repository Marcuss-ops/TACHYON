#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include "tachyon/core/media/media_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

namespace tachyon { enum class AlphaMode; }
namespace tachyon::renderer2d { class SurfaceRGBA; }

namespace tachyon::media {

enum class ResolutionPurpose {
    Playback,
    Export,
    Analysis,
    ProxyGeneration
};

struct AssetRequest {
    std::string id;
    std::filesystem::path path;
    AssetKind kind{AssetKind::Unknown};
    ResolutionPurpose purpose{ResolutionPurpose::Playback};
    uint32_t target_width{0};
    ::tachyon::AlphaMode alpha_mode;
    double time_seconds{0.0};
};

/**
 * Result of resolving an asset reference to concrete filesystem paths
 * and loaded runtime resources.
 */
struct ResolvedAsset {
    std::string id;
    std::string type_name;
    std::filesystem::path source_path;
    std::filesystem::path runtime_path;
    AssetType type{AssetType::PROJECT};
    AssetKind kind{AssetKind::Unknown};
    bool exists{false};
    bool uses_proxy{false};
    
    // Loaded runtime data
    std::shared_ptr<const renderer2d::SurfaceRGBA> image_frame;

    DiagnosticBag diagnostics;
};

} // namespace tachyon::media
