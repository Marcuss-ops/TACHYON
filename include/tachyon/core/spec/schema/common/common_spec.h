#pragma once

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
    Video = 8,
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

/**
 * @brief Compositing blend modes for layers.
 *
 * Must be kept in sync with renderer2d::BlendMode (blending.h).
 */
enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity,
    Additive,
    LinearDodge,
    Subtract,
    Divide,
    LinearBurn,
    VividLight,
    LinearLight,
    PinLight,
    HardMix,
    DarkerColor,
    LighterColor,
    StencilAlpha,
    StencilLuma,
    SilhouetteAlpha,
    SilhouetteLuma,
    AlphaAdd,
    LuminescentPremult,
    Count             ///< Sentinel value — not a real blend mode
};

/**
 * @brief Direction for transitions and animations.
 */
enum class Direction {
    None,
    Left,
    Right,
    Up,
    Down,
    In,
    Out
};

enum class AudioBandType {
    Bass,
    Mid,
    High,
    Presence,
    Rms
};

struct AssetHandle {
    std::string id;
};

/**
 * @brief Unified timing for layers.
 * 
 * start: Time in composition when layer begins (seconds)
 * duration: Visible duration in composition (seconds)
 * source_in: Starting offset within the source asset (seconds)
 * source_out: Ending offset within the source asset (seconds)
 */
struct LayerTiming {
    double start{0.0};
    double duration{10.0};
    double source_in{0.0};
    double source_out{10.0};
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

} // namespace tachyon