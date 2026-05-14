#include "tachyon/renderer2d/effects/core/effect_param_access.h"

namespace tachyon::renderer2d {

bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (auto it = params.params.find(key); it != params.params.end()) {
            if (std::holds_alternative<float>(it->second)) return true;
        }
    }
    return false;
}

bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (auto it = params.params.find(key); it != params.params.end()) {
            if (std::holds_alternative<Color>(it->second)) return true;
        }
    }
    return false;
}

float get_scalar(const EffectParams& params, const std::string& key, float fallback) {
    auto it = params.params.find(key);
    if (it != params.params.end()) {
        if (const auto* val = std::get_if<float>(&it->second)) return *val;
    }
    return fallback;
}

Color get_color(const EffectParams& params, const std::string& key, Color fallback) {
    auto it = params.params.find(key);
    if (it != params.params.end()) {
        if (const auto* val = std::get_if<Color>(&it->second)) return *val;
    }
    return fallback;
}

std::string get_string(const EffectParams& params, const std::string& key, const std::string& fallback) {
    auto it = params.params.find(key);
    if (it != params.params.end()) {
        if (const auto* val = std::get_if<std::string>(&it->second)) return *val;
    }
    return fallback;
}

} // namespace tachyon::renderer2d
