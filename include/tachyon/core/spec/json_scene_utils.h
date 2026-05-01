#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <nlohmann/json.hpp>

namespace tachyon {
namespace spec {

using json = nlohmann::json;

// Serialize project metadata (common to scene spec and reports)
inline json serialize_project(const ProjectSpec& project) {
    json j = {
        {"id", project.id},
        {"name", project.name},
        {"authoring_tool", project.authoring_tool}
    };
    if (project.root_seed.has_value()) {
        j["root_seed"] = *project.root_seed;
    }
    return j;
}

} // namespace spec
} // namespace tachyon
