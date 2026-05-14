#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/api.h"
#include <vector>
#include <string>

namespace tachyon::runtime {

/**
 * @brief Result of a pre-render validation check.
 */
struct PreflightResult {
    bool success{true};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void add_error(std::string msg) {
        success = false;
        errors.push_back(std::move(msg));
    }

    void add_warning(std::string msg) {
        warnings.push_back(std::move(msg));
    }
};

/**
 * @brief Validates a scene before rendering to ensure all assets, effects, and 
 * configurations are valid for the current execution environment.
 */
class TACHYON_API RenderPreflight {
public:
    static PreflightResult validate(const SceneSpec& scene);
};

} // namespace tachyon::runtime
