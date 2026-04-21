#pragma once
#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/runtime/execution/frame_hasher.h"
#include "tachyon/renderer2d/framebuffer.h"
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace tachyon::test {

struct FrameMetadata {
    uint32_t index;
    uint64_t hash;
    std::string png_path;
};

struct SceneMetadata {
    std::string scene_id;
    std::vector<FrameMetadata> frames;
};

class VisualGoldenVerifier {
public:
    explicit VisualGoldenVerifier(std::filesystem::path root_path);

    /**
     * @brief Verifies a rendered frame against the golden baseline.
     * @param scene_id Unique identifier for the canonical scene.
     * @param frame_index The frame number within the scene.
     * @param surface The rendered pixel data.
     * @param update_mode If true, overwrites the golden baseline with the current result.
     * @return True if the hash matches (or was updated), false otherwise.
     */
    bool verify_frame(
        const std::string& scene_id,
        uint32_t frame_index,
        const renderer2d::SurfaceRGBA& surface,
        bool update_mode = false);

    const std::string& last_error() const { return m_last_error; }

private:
    bool load_metadata(const std::string& scene_id, SceneMetadata& out_metadata);
    bool save_metadata(const SceneMetadata& metadata);
    std::filesystem::path get_scene_dir(const std::string& scene_id) const;
    std::filesystem::path get_diff_dir(const std::string& scene_id) const;

    std::filesystem::path m_root_path;
    std::string m_last_error;
};

} // namespace tachyon::test
