#include "tachyon/runtime/execution/batch_runner/batch_registry.h"

namespace tachyon::runtime {

BatchRegistry& BatchRegistry::instance() {
    static BatchRegistry registry;
    return registry;
}

void BatchRegistry::register_scene(const std::string& id, SceneFactory factory) {
    m_scenes[id] = std::move(factory);
}

void BatchRegistry::register_job(const std::string& id, RenderJobFactory factory) {
    m_jobs[id] = std::move(factory);
}

std::optional<SceneSpec> BatchRegistry::create_scene(const std::string& id) const {
    auto it = m_scenes.find(id);
    if (it != m_scenes.end()) {
        return it->second();
    }
    return std::nullopt;
}

std::optional<RenderJob> BatchRegistry::create_job(const std::string& id) const {
    auto it = m_jobs.find(id);
    if (it != m_jobs.end()) {
        return it->second();
    }
    return std::nullopt;
}

std::vector<std::string> BatchRegistry::list_scenes() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : m_scenes) ids.push_back(id);
    return ids;
}

std::vector<std::string> BatchRegistry::list_jobs() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : m_jobs) ids.push_back(id);
    return ids;
}

} // namespace tachyon::runtime
