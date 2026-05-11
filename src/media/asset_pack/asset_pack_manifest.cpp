#include "tachyon/media/asset_pack/asset_pack_manifest.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// Use the nlohmann JSON library bundled with tinygltf.
#include "json.hpp"

using json = nlohmann::json;

namespace tachyon::media {
namespace {

std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Non-printable control characters
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // namespace

bool write_asset_pack_manifest(const AssetPackManifest& manifest, const std::string& path) {
    std::ofstream file(path);
    if (!file) {
        std::cerr << "Error: cannot open manifest file for writing: " << path << std::endl;
        return false;
    }

    auto write_str = [](const std::string& s) -> std::string {
        return "\"" + escape_json(s) + "\"";
    };

    file << "{\n";
    file << "  \"version\": " << write_str(manifest.version) << ",\n";

    auto write_array = [&file, &write_str](const std::string& name, const auto& items, auto write_item) {
        file << "  \"" << name << "\": [\n";
        for (std::size_t i = 0; i < items.size(); ++i) {
            file << "    {\n";
            write_item(file, items[i]);
            file << "    }" << (i + 1 < items.size() ? "," : "") << "\n";
        }
        file << "  ]";
    };

    write_array("meshes", manifest.meshes, [&write_str](std::ostream& out, const PackedMeshEntry& m) {
        out << "      \"id\": " << write_str(m.id) << ",\n"
            << "      \"source_path\": " << write_str(m.source_path) << ",\n"
            << "      \"packed_path\": " << write_str(m.packed_path) << ",\n"
            << "      \"codec\": " << write_str(m.codec) << ",\n"
            << "      \"source_bytes\": " << m.source_bytes << ",\n"
            << "      \"packed_bytes\": " << m.packed_bytes << ",\n"
            << "      \"texture_ids\": [";
        for (std::size_t i = 0; i < m.texture_ids.size(); ++i) {
            out << write_str(m.texture_ids[i]) << (i + 1 < m.texture_ids.size() ? ", " : "");
        }
        out << "]\n";
    });
    file << ",\n";

    write_array("textures", manifest.textures, [&write_str](std::ostream& out, const PackedTextureEntry& t) {
        out << "      \"id\": " << write_str(t.id) << ",\n"
            << "      \"source_path\": " << write_str(t.source_path) << ",\n"
            << "      \"packed_path\": " << write_str(t.packed_path) << ",\n"
            << "      \"codec\": " << write_str(t.codec) << ",\n"
            << "      \"source_bytes\": " << t.source_bytes << ",\n"
            << "      \"packed_bytes\": " << t.packed_bytes << "\n";
    });
    file << ",\n";

    write_array("fonts", manifest.fonts, [&write_str](std::ostream& out, const PackedFontEntry& f) {
        out << "      \"id\": " << write_str(f.id) << ",\n"
            << "      \"source_path\": " << write_str(f.source_path) << ",\n"
            << "      \"packed_path\": " << write_str(f.packed_path) << ",\n"
            << "      \"source_bytes\": " << f.source_bytes << ",\n"
            << "      \"packed_bytes\": " << f.packed_bytes << "\n";
    });
    file << "\n}\n";

    if (!file) {
        std::cerr << "Error: failed to write manifest file: " << path << std::endl;
        return false;
    }

    return true;
}

bool read_asset_pack_manifest(const std::string& path, AssetPackManifest& out) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: cannot open manifest file for reading: " << path << std::endl;
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::parse_error& e) {
        std::cerr << "Error: failed to parse manifest JSON: " << e.what() << std::endl;
        return false;
    }

    // Validate version
    if (root.contains("version") && root["version"].is_string()) {
        out.version = root["version"].get<std::string>();
    } else {
        std::cerr << "Error: manifest missing required field 'version'" << std::endl;
        return false;
    }

    // Helper to parse a path string: reject absolute paths and ".."
    auto parse_path = [](const json& obj, const std::string& key) -> std::string {
        if (!obj.contains(key) || !obj[key].is_string()) return {};
        std::string val = obj[key].get<std::string>();
        // Reject absolute paths
        if (!val.empty() && val[0] == '/') return {};
        // Reject paths containing ".."
        if (val.find("..") != std::string::npos) return {};
        return val;
    };

    // Parse meshes
    if (root.contains("meshes") && root["meshes"].is_array()) {
        for (const auto& item : root["meshes"]) {
            if (!item.is_object()) continue;
            PackedMeshEntry entry;
            if (item.contains("id") && item["id"].is_string()) {
                entry.id = item["id"].get<std::string>();
            } else {
                continue; // id is required
            }
            entry.source_path = parse_path(item, "source_path");
            entry.packed_path = parse_path(item, "packed_path");
            if (item.contains("codec") && item["codec"].is_string()) {
                entry.codec = item["codec"].get<std::string>();
            }
            if (item.contains("source_bytes") && item["source_bytes"].is_number()) {
                entry.source_bytes = item["source_bytes"].get<std::uint64_t>();
            }
            if (item.contains("packed_bytes") && item["packed_bytes"].is_number()) {
                entry.packed_bytes = item["packed_bytes"].get<std::uint64_t>();
            }
            // Parse texture_ids
            if (item.contains("texture_ids") && item["texture_ids"].is_array()) {
                for (const auto& tid : item["texture_ids"]) {
                    if (tid.is_string()) {
                        entry.texture_ids.push_back(tid.get<std::string>());
                    }
                }
            }
            out.meshes.push_back(std::move(entry));
        }
    }

    // Parse textures
    if (root.contains("textures") && root["textures"].is_array()) {
        for (const auto& item : root["textures"]) {
            if (!item.is_object()) continue;
            PackedTextureEntry entry;
            if (item.contains("id") && item["id"].is_string()) {
                entry.id = item["id"].get<std::string>();
            } else {
                continue;
            }
            entry.source_path = parse_path(item, "source_path");
            entry.packed_path = parse_path(item, "packed_path");
            if (item.contains("codec") && item["codec"].is_string()) {
                entry.codec = item["codec"].get<std::string>();
            }
            if (item.contains("source_bytes") && item["source_bytes"].is_number()) {
                entry.source_bytes = item["source_bytes"].get<std::uint64_t>();
            }
            if (item.contains("packed_bytes") && item["packed_bytes"].is_number()) {
                entry.packed_bytes = item["packed_bytes"].get<std::uint64_t>();
            }
            out.textures.push_back(std::move(entry));
        }
    }

    // Parse fonts
    if (root.contains("fonts") && root["fonts"].is_array()) {
        for (const auto& item : root["fonts"]) {
            if (!item.is_object()) continue;
            PackedFontEntry entry;
            if (item.contains("id") && item["id"].is_string()) {
                entry.id = item["id"].get<std::string>();
            } else {
                continue;
            }
            entry.source_path = parse_path(item, "source_path");
            entry.packed_path = parse_path(item, "packed_path");
            if (item.contains("source_bytes") && item["source_bytes"].is_number()) {
                entry.source_bytes = item["source_bytes"].get<std::uint64_t>();
            }
            if (item.contains("packed_bytes") && item["packed_bytes"].is_number()) {
                entry.packed_bytes = item["packed_bytes"].get<std::uint64_t>();
            }
            out.fonts.push_back(std::move(entry));
        }
    }

    return true;
}

} // namespace tachyon::media