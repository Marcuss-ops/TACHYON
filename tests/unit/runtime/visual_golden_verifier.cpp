#include "visual_golden_verifier.h"
#include <fstream>
#include <iostream>

namespace tachyon::test {

VisualGoldenVerifier::VisualGoldenVerifier(std::filesystem::path root_path)
    : m_root_path(std::move(root_path)) {
    std::error_code ec;
    std::filesystem::create_directories(m_root_path, ec);
}

bool VisualGoldenVerifier::verify_frame(
    const std::string& scene_id,
    uint32_t frame_index,
    const renderer2d::SurfaceRGBA& surface,
    bool update_mode) {

    m_last_error.clear();
    FrameHasher hasher;
    const uint64_t current_hash = hasher.hash_pixels(surface.pixels());

    SceneMetadata metadata;
    metadata.scene_id = scene_id;
    load_metadata(scene_id, metadata);

    const std::filesystem::path scene_dir = get_scene_dir(scene_id);
    const std::string png_name = "frame_" + std::to_string(frame_index) + ".png";
    const std::filesystem::path png_path = scene_dir / png_name;

    if (update_mode) {
        std::error_code ec;
        std::filesystem::create_directories(scene_dir, ec);
        
        if (!surface.save_png(png_path)) {
            m_last_error = "Failed to save golden PNG: " + png_path.string();
            return false;
        }

        // Update or add frame metadata
        bool found = false;
        for (auto& f : metadata.frames) {
            if (f.index == frame_index) {
                f.hash = current_hash;
                f.png_path = png_name;
                found = true;
                break;
            }
        }
        if (!found) {
            metadata.frames.push_back({frame_index, current_hash, png_name});
        }

        return save_metadata(metadata);
    }

    // Verify
    bool frame_found = false;
    uint64_t expected_hash = 0;
    for (const auto& f : metadata.frames) {
        if (f.index == frame_index) {
            expected_hash = f.hash;
            frame_found = true;
            break;
        }
    }

    if (!frame_found) {
        m_last_error = "No golden baseline found for scene '" + scene_id + "' frame " + std::to_string(frame_index);
        return false;
    }

    if (current_hash != expected_hash) {
        const std::filesystem::path diff_dir = get_diff_dir(scene_id);
        std::error_code ec;
        std::filesystem::create_directories(diff_dir, ec);
        
        const std::filesystem::path actual_path = diff_dir / ("frame_" + std::to_string(frame_index) + "_actual.png");
        surface.save_png(actual_path);
        
        m_last_error = "Hash mismatch for " + scene_id + ":" + std::to_string(frame_index) + 
                       ". Expected: 0x" + std::to_string(expected_hash) + 
                       ", Got: 0x" + std::to_string(current_hash) + 
                       ". Actual saved to: " + actual_path.string();
        return false;
    }

    return true;
}

bool VisualGoldenVerifier::load_metadata(const std::string& scene_id, SceneMetadata& out_metadata) {
    const std::filesystem::path path = get_scene_dir(scene_id) / "metadata.json";
    std::ifstream f(path);
    if (!f.is_open()) return false;

    try {
        nlohmann::json j;
        f >> j;
        out_metadata.scene_id = j.value("scene_id", scene_id);
        if (j.contains("frames")) {
            for (const auto& fj : j["frames"]) {
                out_metadata.frames.push_back({
                    fj.value("index", 0u),
                    fj.value("hash", 0ull),
                    fj.value("png_path", "")
                });
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool VisualGoldenVerifier::save_metadata(const SceneMetadata& metadata) {
    const std::filesystem::path path = get_scene_dir(metadata.scene_id) / "metadata.json";
    nlohmann::json j;
    j["scene_id"] = metadata.scene_id;
    nlohmann::json frames = nlohmann::json::array();
    for (const auto& f : metadata.frames) {
        frames.push_back({
            {"index", f.index},
            {"hash", f.hash},
            {"png_path", f.png_path}
        });
    }
    j["frames"] = frames;

    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << j.dump(2);
    return true;
}

std::filesystem::path VisualGoldenVerifier::get_scene_dir(const std::string& scene_id) const {
    return m_root_path / scene_id;
}

std::filesystem::path VisualGoldenVerifier::get_diff_dir(const std::string& scene_id) const {
    return m_root_path / "diffs" / scene_id;
}

} // namespace tachyon::test
