#include "tachyon/core/animation/easing_parse.h"
#include <unordered_map>

namespace tachyon::animation {

EasingPreset parse_easing_preset(const std::string& name) {
    static const std::unordered_map<std::string, EasingPreset> easing_map = {
        {"none", EasingPreset::None},
        {"linear", EasingPreset::None},
        {"easeIn", EasingPreset::EaseIn},
        {"easeOut", EasingPreset::EaseOut},
        {"easeInOut", EasingPreset::EaseInOut},
        {"spring", EasingPreset::Spring},
        {"custom", EasingPreset::Custom}
    };

    auto it = easing_map.find(name);
    return (it != easing_map.end()) ? it->second : EasingPreset::None;
}

EasingPreset parse_easing_preset(const nlohmann::json& j) {
    if (j.is_string()) {
        return parse_easing_preset(j.get<std::string>());
    }
    return EasingPreset::None;
}

bool parse_bezier(const nlohmann::json& j, float& x1, float& y1, float& x2, float& y2) {
    if (!j.is_array() || j.size() < 4) return false;
    x1 = j[0].get<float>();
    y1 = j[1].get<float>();
    x2 = j[2].get<float>();
    y2 = j[3].get<float>();
    return true;
}

CubicBezierEasing parse_bezier(const nlohmann::json& j) {
    float x1, y1, x2, y2;
    if (parse_bezier(j, x1, y1, x2, y2)) {
        return {static_cast<double>(x1), static_cast<double>(y1), static_cast<double>(x2), static_cast<double>(y2)};
    }
    return CubicBezierEasing::linear();
}

} // namespace tachyon::animation
