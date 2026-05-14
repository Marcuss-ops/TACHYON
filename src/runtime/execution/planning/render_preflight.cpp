#include "tachyon/runtime/execution/planning/render_preflight.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include <filesystem>

namespace tachyon::runtime {

PreflightResult RenderPreflight::validate(const SceneSpec& scene) {
    PreflightResult result;
    
    // 1. Structural Validation via SceneValidator
    core::SceneValidator validator;
    auto val_res = validator.validate(scene);
    
    for (const auto& issue : val_res.issues) {
        if (issue.severity == core::ValidationIssue::Severity::Fatal || 
            issue.severity == core::ValidationIssue::Severity::Error) {
            result.add_error("[" + issue.path + "] " + issue.message);
        } else {
            result.add_warning("[" + issue.path + "] " + issue.message);
        }
    }
    
    // 2. Runtime Environment Validation
    // Check asset existence on disk
    for (const auto& asset : scene.assets) {
        if (!asset.path.empty()) {
            if (!std::filesystem::exists(asset.path)) {
                result.add_error("Asset file not found: " + asset.path + " (ID: " + asset.id + ")");
            }
        }
    }
    
    // 3. Effect availability check (placeholder - requires registry access)
    // In a real implementation, we'd pass the EffectRegistry to this method.
    
    return result;
}

} // namespace tachyon::runtime
