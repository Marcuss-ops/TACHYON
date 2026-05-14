#pragma once

#include "tachyon/core/spec/schema/effects/effect_spec.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include <string>
#include <vector>

namespace tachyon::renderer2d {

struct EffectValidationIssue {
    enum class Severity { Warning, Error };
    Severity severity;
    std::string param_id;
    std::string message;
};

struct EffectValidationResult {
    bool valid{true};
    std::vector<EffectValidationIssue> issues;
};

EffectValidationResult validate_effect(const EffectSpec& spec, const EffectRegistry& registry);

} // namespace tachyon::renderer2d
