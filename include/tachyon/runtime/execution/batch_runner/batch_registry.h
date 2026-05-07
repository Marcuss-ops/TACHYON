#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include <string>
#include <functional>
#include <map>
#include <optional>

namespace tachyon::runtime {

using SceneFactory = std::function<SceneSpec()>;
using RenderJobFactory = std::function<RenderJob()>;

/**
 * @brief Registry for C++ based scene and job factories for batch rendering.
 * Replaces legacy JSON loading with deterministic C++ code.
 */
class BatchRegistry {
public:
    static BatchRegistry& instance();

    void register_scene(const std::string& id, SceneFactory factory);
    void register_job(const std::string& id, RenderJobFactory factory);

    std::optional<SceneSpec> create_scene(const std::string& id) const;
    std::optional<RenderJob> create_job(const std::string& id) const;

    std::vector<std::string> list_scenes() const;
    std::vector<std::string> list_jobs() const;

private:
    BatchRegistry() = default;
    
    std::map<std::string, SceneFactory> m_scenes;
    std::map<std::string, RenderJobFactory> m_jobs;
};

} // namespace tachyon::runtime
