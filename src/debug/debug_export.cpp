#include "tachyon/debug/debug_export.h"

#include "stb_image_write.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace tachyon::debug {

bool export_rgba_to_png(const std::string& path, const uint8_t* data, int width, int height) {
    if (!data || width <= 0 || height <= 0) {
        std::cerr << "[DebugExport] Invalid parameters for RGBA export\n";
        return false;
    }

    // Create directory if it doesn't exist
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    // Export as 32-bit RGBA PNG
    int result = stbi_write_png(path.c_str(), width, height, 4, data, width * 4);
    if (result == 0) {
        std::cerr << "[DebugExport] Failed to write RGBA PNG: " << path << '\n';
        return false;
    }

    std::cerr << "[DebugExport] Exported RGBA to: " << path << '\n';
    return true;
}

bool export_float_buffer_to_png(const std::string& path, const float* data, int width, int height, const std::string& channel_name) {
    if (!data || width <= 0 || height <= 0) {
        std::cerr << "[DebugExport] Invalid parameters for float buffer export\n";
        return false;
    }

    // Convert float buffer to 8-bit grayscale
    std::vector<uint8_t> grayscale(width * height);
    for (int i = 0; i < width * height; ++i) {
        float val = data[i];
        // Clamp to [0, 1] and convert to [0, 255]
        val = (val < 0.0f) ? 0.0f : (val > 1.0f) ? 1.0f : val;
        grayscale[i] = static_cast<uint8_t>(val * 255.0f);
    }

    // Create directory if it doesn't exist
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    // Export as 8-bit grayscale PNG
    int result = stbi_write_png(path.c_str(), width, height, 1, grayscale.data(), width);
    if (result == 0) {
        std::cerr << "[DebugExport] Failed to write float buffer PNG: " << path << '\n';
        return false;
    }

    std::cerr << "[DebugExport] Exported float buffer '" << channel_name << "' to: " << path << '\n';
    return true;
}

bool export_json(const std::string& path, const std::string& json_string) {
    // Create directory if it doesn't exist
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[DebugExport] Failed to open JSON file: " << path << '\n';
        return false;
    }

    file << json_string;
    file.close();

    if (!file.good()) {
        std::cerr << "[DebugExport] Failed to write JSON file: " << path << '\n';
        return false;
    }

    std::cerr << "[DebugExport] Exported JSON to: " << path << '\n';
    return true;
}

std::string ensure_snapshot_dir(const std::string& base_path) {
    std::filesystem::path dir(base_path);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        std::cerr << "[DebugExport] Created snapshot directory: " << dir.string() << '\n';
    }
    return dir.string();
}

} // namespace tachyon::debug
