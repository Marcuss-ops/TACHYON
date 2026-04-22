#pragma once

#include "tachyon/core/math/vector3.h"
#include <string>
#include <cstdint>

namespace tachyon {

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] math::Vector3 to_vector3() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }
};

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
    Unknown
};

enum class AudioBandType {
    Bass,
    Mid,
    High,
    Presence,
    Rms
};

} // namespace tachyon
