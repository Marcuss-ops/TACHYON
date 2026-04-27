#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>

namespace tachyon {

/**
 * @brief Manages loading and caching of Tachyon templates.
 * 
 * The registry ensures that each unique template file is only parsed once
 * and can be reused across multiple scene compilations.
 */
class TemplateRegistry {
public:
    TemplateRegistry() = default;
    ~TemplateRegistry() = default;

    /**
     * @brief Load a template from the given path or return the cached version.
     * @param path Filesystem path to the .tachyon.json template file.
     * @return Shared pointer to the parsed SceneSpec.
     */
    std::shared_ptr<SceneSpec> get_template(const std::filesystem::path& path);

    /**
     * @brief Clear the template cache.
     */
    void clear_cache();

    /**
     * @brief Check if a template is already in cache.
     */
    bool is_cached(const std::filesystem::path& path) const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<SceneSpec>> m_cache;
};

} // namespace tachyon
