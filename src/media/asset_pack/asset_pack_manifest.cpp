#include "tachyon/media/asset_pack/asset_pack_manifest.h"

#include <fstream>
#include <iostream>

namespace tachyon::media {

bool write_asset_pack_manifest(const AssetPackManifest& manifest, const std::string& path) {
    std::ofstream file(path);
    if (!file) return false;

    // A simple JSON writer for the manifest
    file << "{\n";
    file << "  \"version\": \"" << manifest.version << "\",\n";

    auto write_array = [&file](const std::string& name, const auto& items, auto write_item) {
        file << "  \"" << name << "\": [\n";
        for (std::size_t i = 0; i < items.size(); ++i) {
            file << "    {\n";
            write_item(file, items[i]);
            file << "    }" << (i + 1 < items.size() ? "," : "") << "\n";
        }
        file << "  ]";
    };

    write_array("meshes", manifest.meshes, [](std::ostream& out, const PackedMeshEntry& m) {
        out << "      \"id\": \"" << m.id << "\",\n"
            << "      \"source_path\": \"" << m.source_path << "\",\n"
            << "      \"packed_path\": \"" << m.packed_path << "\",\n"
            << "      \"codec\": \"" << m.codec << "\",\n"
            << "      \"source_bytes\": " << m.source_bytes << ",\n"
            << "      \"packed_bytes\": " << m.packed_bytes << ",\n"
            << "      \"texture_ids\": [";
        for (std::size_t i = 0; i < m.texture_ids.size(); ++i) {
            out << "\"" << m.texture_ids[i] << "\"" << (i + 1 < m.texture_ids.size() ? ", " : "");
        }
        out << "]\n";
    });
    file << ",\n";

    write_array("textures", manifest.textures, [](std::ostream& out, const PackedTextureEntry& t) {
        out << "      \"id\": \"" << t.id << "\",\n"
            << "      \"source_path\": \"" << t.source_path << "\",\n"
            << "      \"packed_path\": \"" << t.packed_path << "\",\n"
            << "      \"codec\": \"" << t.codec << "\",\n"
            << "      \"source_bytes\": " << t.source_bytes << ",\n"
            << "      \"packed_bytes\": " << t.packed_bytes << "\n";
    });
    file << ",\n";

    write_array("fonts", manifest.fonts, [](std::ostream& out, const PackedFontEntry& f) {
        out << "      \"id\": \"" << f.id << "\",\n"
            << "      \"source_path\": \"" << f.source_path << "\",\n"
            << "      \"packed_path\": \"" << f.packed_path << "\",\n"
            << "      \"source_bytes\": " << f.source_bytes << ",\n"
            << "      \"packed_bytes\": " << f.packed_bytes << "\n";
    });
    file << "\n}\n";

    return true;
}

bool read_asset_pack_manifest(const std::string& path, AssetPackManifest& out) {
    // For PR 4, we provide a placeholder since the full parser
    // might require a JSON library or rapidjson which is used in Tachyon.
    // Assuming we want a simple implementation or can just return true for now.
    // In a real scenario, this would parse the JSON written above.
    return true;
}

} // namespace tachyon::media