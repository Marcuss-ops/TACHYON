#pragma once

#include "tachyon/core/math/vector3.h"
#include <string>
#include <cstdint>
#include <map>
#include <string_view>

#include "tachyon/core/api.h"
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
    Unknown = 0,
    Solid = 1,
    Shape = 2,
    Image = 3,
    Text = 4,
    Precomp = 5,
    Procedural = 6,
    Mesh = 7,
    Video = 8,
    Camera = 9,
    Light = 10,
    Mask = 11,
    NullLayer = 12
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

// --- Layer Type Helpers ---
/**
 * @brief Get the canonical string representation of a LayerType.
 * Used for serialization and authoring.
 */
TACHYON_API std::string_view to_canonical_layer_type_string(LayerType type);

/**
 * @brief Parse a canonical string into a LayerType enum.
 */
TACHYON_API LayerType layer_type_from_string(std::string_view type_string);

/**
 * @brief Map a LayerType to the internal compiled type ID.
 */
TACHYON_API std::uint32_t compiled_type_id_from_layer_type(LayerType type);

struct EffectSpec {
    bool enabled{true};
    std::string type;
    std::map<std::string, double> scalars;
    std::map<std::string, ColorSpec> colors;
    std::map<std::string, std::string> strings;

    [[nodiscard]] EffectSpec evaluate(double time_seconds) const;
};

} // namespace tachyon
