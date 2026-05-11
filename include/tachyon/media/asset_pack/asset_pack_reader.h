#pragma once

#include "tachyon/media/asset_pack/asset_pack_manifest.h"

#include <string>

namespace tachyon::media {

class AssetPackReader {
public:
    bool open(const std::string& pack_dir);
    const AssetPackManifest& manifest() const;

private:
    std::string pack_dir_;
    AssetPackManifest manifest_;
};

} // namespace tachyon::media