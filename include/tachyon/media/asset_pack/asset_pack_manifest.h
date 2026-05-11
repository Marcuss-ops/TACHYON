#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::media {

struct PackedMeshEntry {
    std::string id;
    std::string source_path;
    std::string packed_path;
    std::string codec;
    std::uint64_t source_bytes = 0;
    std::uint64_t packed_bytes = 0;
};

struct PackedTextureEntry {
    std::string id;
    std::string source_path;
    std::string packed_path;
    std::string codec;
    std::uint64_t source_bytes = 0;
    std::uint64_t packed_bytes = 0;
};

struct PackedFontEntry {
    std::string id;
    std::string source_path;
    std::string packed_path;
    std::uint64_t source_bytes = 0;
    std::uint64_t packed_bytes = 0;
};

struct AssetPackManifest {
    std::string version = "0.1";
    std::vector<PackedMeshEntry> meshes;
    std::vector<PackedTextureEntry> textures;
    std::vector<PackedFontEntry> fonts;
};

bool write_asset_pack_manifest(const AssetPackManifest& manifest, const std::string& path);
bool read_asset_pack_manifest(const std::string& path, AssetPackManifest& out);

} // namespace tachyon::media