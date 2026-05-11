#pragma once

#include "tachyon/media/asset_pack/asset_pack_manifest.h"
#include "tachyon/media/compression/mesh_compressor.h"
#include "tachyon/media/compression/texture_compressor.h"

#include <string>

namespace tachyon::media {

struct AssetPackBuilderOptions {
    std::string input_path;
    std::string output_dir;
    std::string mesh_codec = "none";
    std::string texture_codec = "none";
};

struct AssetPackBuilderResult {
    bool ok = false;
    std::string error;
    AssetPackManifest manifest;
};

class AssetPackBuilder {
public:
    AssetPackBuilderResult build(const AssetPackBuilderOptions& options);
};

} // namespace tachyon::media