#include "tachyon/renderer2d/effects/effect_validation.h"
#include <algorithm>

namespace tachyon::renderer2d {

EffectValidationResult validate_effect(const EffectSpec& spec, const EffectRegistry& registry) {
    EffectValidationResult result;
    
    const auto* desc = registry.find(spec.type);
    if (!desc) {
        result.valid = false;
        result.issues.push_back({
            EffectValidationIssue::Severity::Error,
            "",
            "Effect type '" + spec.type + "' not found in registry."
        });
        return result;
    }

    // Check for unknown parameters
    for (const auto& [param_id, value] : spec.params) {
        bool found = false;
        for (const auto& def : desc->schema.params) {
            if (def.id == param_id) {
                found = true;
                // Type validation would go here
                break;
            }
        }
        
        if (!found) {
            result.valid = false;
            result.issues.push_back({
                EffectValidationIssue::Severity::Error,
                param_id,
                "Unknown parameter '" + param_id + "' for effect type '" + spec.type + "'."
            });
        }
    }

    // Check for required parameters
    for (const auto& def : desc->schema.params) {
        if (def.required && !spec.params.contains(def.id)) {
            result.valid = false;
            result.issues.push_back({
                EffectValidationIssue::Severity::Error,
                def.id,
                "Required parameter '" + def.id + "' is missing."
            });
        }
    }

    return result;
}

} // namespace tachyon::renderer2d
