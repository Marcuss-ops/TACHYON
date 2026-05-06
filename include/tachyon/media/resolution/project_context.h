#pragma once
#include <filesystem>

namespace tachyon::media {

/**
 * @brief Unified context defining all search roots for a project.
 * This is the single source of truth for path resolution policy.
 */
struct ProjectResolutionContext {
    std::filesystem::path project_root;    ///< The folder containing the scene/project file
    std::filesystem::path assets_root;     ///< Global or project-local assets folder
    std::filesystem::path sfx_root;        ///< Root for audio/SFX
    std::filesystem::path fonts_root;      ///< Root for font files

    /**
     * @brief Creates a context by deriving roots from a scene file path.
     */
    static ProjectResolutionContext from_scene(const std::filesystem::path& scene_path);
};

} // namespace tachyon::media
