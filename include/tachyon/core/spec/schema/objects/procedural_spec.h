#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace tachyon {

/**
 * @brief Specification for procedural background generation.
 * 
 * Defines parameters for noise-based and geometric patterns rendered in real-time.
 */
struct ProceduralSpec {
    std::string kind;           ///< Pattern type: "aura", "grid", "grid_lines", "stars", "stripes", "waves", "noise"
    uint64_t seed{0};           ///< Random seed for pattern generation

    // Core Parameters
    AnimatedScalarSpec frequency;
    AnimatedScalarSpec speed;
    AnimatedScalarSpec amplitude;
    AnimatedScalarSpec scale;
    AnimatedScalarSpec angle;

    // Colors
    AnimatedColorSpec color_a;
    AnimatedColorSpec color_b;
    AnimatedColorSpec color_c;

    // Grid & Geometry
    std::string shape{"square"}; ///< "square" or "circle"
    AnimatedScalarSpec spacing;
    AnimatedScalarSpec border_width;
    AnimatedColorSpec border_color;

    // Warp / Distortion
    AnimatedScalarSpec warp_strength;
    AnimatedScalarSpec warp_frequency;
    AnimatedScalarSpec warp_speed;

    // Post-Process
    AnimatedScalarSpec grain_amount;
    AnimatedScalarSpec grain_scale;
    AnimatedScalarSpec scanline_intensity;
    AnimatedScalarSpec scanline_frequency;
    AnimatedScalarSpec contrast;
    AnimatedScalarSpec gamma;
    AnimatedScalarSpec saturation;
    AnimatedScalarSpec softness;

    // Advanced / Noise Specific
    AnimatedScalarSpec octave_decay;
    AnimatedScalarSpec band_height;
    AnimatedScalarSpec band_spread;

    /**
     * @brief Check if the spec is effectively empty (no kind specified).
     */
    [[nodiscard]] bool empty() const noexcept {
        return kind.empty();
    }
};

// JSON serialization declarations
void to_json(nlohmann::json& j, const ProceduralSpec& spec);
void from_json(const nlohmann::json& j, ProceduralSpec& spec);

} // namespace tachyon
