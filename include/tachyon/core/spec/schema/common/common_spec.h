#pragma once

#include "tachyon/core/math/vector3.h"
#include <string>
#include <cstdint>
#include <map>

#include "tachyon/core/types/color_spec.h"

namespace tachyon {

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

} // namespace tachyon
