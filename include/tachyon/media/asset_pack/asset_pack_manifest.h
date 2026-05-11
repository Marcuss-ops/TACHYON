#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tachyon::media {

template <typename Tag>
struct ManifestPath {
    std::string value;

    ManifestPath() = default;
    ManifestPath(const char* s) : value(s ? s : "") {}
    ManifestPath(std::string s) : value(std::move(s)) {}
    ManifestPath(std::string_view s) : value(s) {}

    ManifestPath& operator=(std::string s) {
        value = std::move(s);
        return *this;
    }

    ManifestPath& operator=(const char* s) {
        value = s ? s : "";
        return *this;
    }

    [[nodiscard]] const std::string& str() const { return value; }
    [[nodiscard]] bool empty() const { return value.empty(); }
    explicit operator bool() const { return !value.empty(); }
    operator std::string_view() const noexcept { return value; }
};

struct SourcePathTag {};
struct PackedPathTag {};

using SourcePath = ManifestPath<SourcePathTag>;
using PackedPath = ManifestPath<PackedPathTag>;

struct PackedMeshEntry {
    std::string id;
    SourcePath source_path;
    PackedPath packed_path;
    std::string codec;
    std::uint64_t source_bytes = 0;
    std::uint64_t packed_bytes = 0;
    std::vector<std::string> texture_ids;
};

struct PackedTextureEntry {
    std::string id;
    SourcePath source_path;
    PackedPath packed_path;
    std::string codec;
    std::uint64_t source_bytes = 0;
    std::uint64_t packed_bytes = 0;
};

struct PackedFontEntry {
    std::string id;
    SourcePath source_path;
    PackedPath packed_path;
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
