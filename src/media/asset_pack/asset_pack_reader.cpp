#include "tachyon/media/asset_pack/asset_pack_reader.h"

namespace tachyon::media {

bool AssetPackReader::open(const std::string& pack_dir) {
    pack_dir_ = pack_dir;
    return read_asset_pack_manifest(pack_dir_ + "/manifest.json", manifest_);
}

const AssetPackManifest& AssetPackReader::manifest() const {
    return manifest_;
}

} // namespace tachyon::media