#include "tachyon/renderer2d/effects/core/effect_param_access.h"

namespace tachyon::renderer2d {

bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.scalars.count(key)) return true;
    }
    return false;
}

bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.colors.count(key)) return true;
    }
    return false;
}

float get_scalar(const EffectParams& params, const std::string& key, float fallback) {
    auto it = params.scalars.find(key);
    return (it != params.scalars.end()) ? it->second : fallback;
}

Color get_color(const EffectParams& params, const std::string& key, Color fallback) {
    auto it = params.colors.find(key);
    return (it != params.colors.end()) ? it->second : fallback;
}

std::string get_string(const EffectParams& params, const std::string& key, const std::string& fallback) {
    auto it = params.strings.find(key);
    return (it != params.strings.end()) ? it->second : fallback;
}

} // namespace tachyon::renderer2d
