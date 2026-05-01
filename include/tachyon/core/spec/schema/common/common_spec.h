#pragma once

#include "tachyon/core/math/vector3.h"
#include <string>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>

#include "tachyon/core/types/color_spec.h"

namespace tachyon {

// ColorSpec moved to tachyon/core/types/color_spec.h

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

enum class TransitionKind {
    None,
    Fade,
    Slide,
    Zoom,
    Flip,
    Wipe,
    Dissolve,
    Custom
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

    [[nodiscard]] EffectSpec evaluate(double time_seconds) const;
};

void to_json(nlohmann::json& j, const EffectSpec& e);
void from_json(const nlohmann::json& j, EffectSpec& e);

void to_json(nlohmann::json& j, const ColorSpec& c);
void from_json(const nlohmann::json& j, ColorSpec& c);

} // namespace tachyon
