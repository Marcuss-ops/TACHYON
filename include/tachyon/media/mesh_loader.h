#pragma once

#include "tachyon/media/mesh_asset.h"
#include "tachyon/runtime/diagnostics.h"
#include <filesystem>
#include <memory>

namespace tachyon::media {

class MeshLoader {
public:
    static std::unique_ptr<MeshAsset> load_from_gltf(
        const std::filesystem::path& path,
        DiagnosticBag* diagnostics = nullptr);
};

} // namespace tachyon::media
