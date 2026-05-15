#include "tachyon/runtime/execution/planning/render_preflight.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/effect_validation.h"
#include "tachyon/transition_registry.h"
#include <filesystem>

namespace tachyon::runtime {

PreflightResult RenderPreflight::validate(
    const SceneSpec& scene,
    const renderer2d::EffectRegistry* effect_registry,
    const TransitionRegistry* transition_registry) {
    
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
    
    // 3. Registry-aware Validation
    for (const auto& comp : scene.compositions) {
        for (const auto& layer : comp.layers) {
            const std::string layer_path = "composition(" + comp.id + ").layer(" + layer.identity.id + ")";
            
            // Check transitions
            if (transition_registry) {
                if (layer.transition_in.transition_id != "none" && 
                    layer.transition_in.transition_id != "tachyon.transition.none") {
                    if (!transition_registry->resolve(layer.transition_in.transition_id)) {
                        result.add_error(layer_path + ": Transition in not found: " + layer.transition_in.transition_id);
                    }
                }
                if (layer.transition_out.transition_id != "none" && 
                    layer.transition_out.transition_id != "tachyon.transition.none") {
                    if (!transition_registry->resolve(layer.transition_out.transition_id)) {
                        result.add_error(layer_path + ": Transition out not found: " + layer.transition_out.transition_id);
                    }
                }
            }
            
            // Check effects
            if (effect_registry) {
                for (std::size_t i = 0; i < layer.effects.size(); ++i) {
                    const auto& effect = layer.effects[i];
                    auto fx_res = ::tachyon::renderer2d::validate_effect(effect, *effect_registry);
                    if (!fx_res.valid) {
                        for (const auto& issue : fx_res.issues) {
                            result.add_error(layer_path + ".effects[" + std::to_string(i) + "]: " + issue.message);
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

} // namespace tachyon::runtime
