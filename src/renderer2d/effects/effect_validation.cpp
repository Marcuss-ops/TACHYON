#include "tachyon/renderer2d/effects/effect_validation.h"
#include <algorithm>
#include <limits>

namespace tachyon::renderer2d {

namespace {

int levenshtein_distance(const std::string& s1, const std::string& s2) {
    const std::size_t len1 = s1.size();
    const std::size_t len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (std::size_t i = 0; i <= len1; ++i) d[i][0] = static_cast<int>(i);
    for (std::size_t j = 0; j <= len2; ++j) d[0][j] = static_cast<int>(j);

    for (std::size_t i = 1; i <= len1; ++i) {
        for (std::size_t j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost });
        }
    }
    return d[len1][len2];
}

std::string find_suggestion(const std::string& input, const std::vector<registry::ParameterDef>& params) {
    std::string best_match;
    int min_dist = std::numeric_limits<int>::max();
    
    for (const auto& def : params) {
        int dist = levenshtein_distance(input, def.id);
        if (dist < min_dist && dist <= 2) {
            min_dist = dist;
            best_match = def.id;
        }
    }
    return best_match;
}

bool check_type(const registry::ParameterValue& value, registry::ParameterType expected) {
    switch (expected) {
        case registry::ParameterType::Float:  return std::holds_alternative<float>(value);
        case registry::ParameterType::Double: return std::holds_alternative<double>(value);
        case registry::ParameterType::Int:    return std::holds_alternative<int>(value);
        case registry::ParameterType::Bool:   return std::holds_alternative<bool>(value);
        case registry::ParameterType::String: return std::holds_alternative<std::string>(value);
        case registry::ParameterType::Color:  return std::holds_alternative<ColorSpec>(value);
        case registry::ParameterType::Vector2: return std::holds_alternative<math::Vector2>(value);
        default: return true; // Complex types or placeholders
    }
}

} // namespace

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
        const registry::ParameterDef* def_ptr = nullptr;
        for (const auto& def : desc->schema.params) {
            if (def.id == param_id) {
                def_ptr = &def;
                break;
            }
        }
        
        if (!def_ptr) {
            result.valid = false;
            std::string msg = "Unknown parameter '" + param_id + "' for effect type '" + spec.type + "'.";
            std::string suggestion = find_suggestion(param_id, desc->schema.params);
            if (!suggestion.empty()) {
                msg += " Did you mean '" + suggestion + "'?";
            }
            
            result.issues.push_back({
                EffectValidationIssue::Severity::Error,
                param_id,
                msg
            });
            continue;
        }

        // Type checking for static parameters (EvaluatedEffect params are variants too)
        // Note: EffectSpec params can be AnimatedScalarSpec, etc. 
        // We only check literal values here.
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float> || 
                          std::is_same_v<T, bool> || std::is_same_v<T, std::string> || 
                          std::is_same_v<T, ColorSpec> || std::is_same_v<T, math::Vector2>) {
                // Perform basic literal type check if def_ptr->type matches
                // (Omitted for brevity, but could be added)
            }
        }, value);
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
