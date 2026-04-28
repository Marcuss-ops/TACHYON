#pragma once

#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/types/colors.h"
#include <string>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>

namespace tachyon {

// ColorSpec extensions
[[nodiscard]] inline math::Vector3 color_spec_to_vector3(const ColorSpec& c) {
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};
}

// JSON serialization declarations (implementations in common_spec_serialize.cpp)
void to_json(nlohmann::json& j, const ColorSpec& c);
void from_json(const nlohmann::json& j, ColorSpec& c);

enum class TrackMatteType {
    None,
    Alpha,
    AlphaInverted,
    Luma,
    LumaInverted
};

enum class LayerType {
    Solid,
    Shape,
    Image,
    Video,
    Text,
    Camera,
    Precomp,
    Light,
    Mask,
    NullLayer,
    Procedural,
    Unknown
};

enum class AudioBandType {
    Bass,
    Mid,
    High,
    Presence,
    Rms
};

struct EffectSpec {
    bool enabled{true};
    std::string type;
    std::map<std::string, double> scalars;
    std::map<std::string, ColorSpec> colors;
    std::map<std::string, std::string> strings;
    
    // Easing for effect progress (e.g., transition t)
    animation::EasingPreset easing_preset{animation::EasingPreset::None};
    animation::CubicBezierEasing custom_easing{animation::CubicBezierEasing::linear()};

    [[nodiscard]] EffectSpec evaluate(double time_seconds) const;
};

void to_json(nlohmann::json& j, const EffectSpec& e);
void from_json(const nlohmann::json& j, EffectSpec& e);

} // namespace tachyon

