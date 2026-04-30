#pragma once

#include "tachyon/core/math/vector3.h"
#include <cstdint>

namespace tachyon::camera {

/**
 * @brief Deterministic camera shake using seeded noise.
 * 
 * This is a transform modifier, not a post effect.
 * It is applied in camera/rig space before world matrix computation.
 * 
 * The noise is deterministic: same seed + same time = same displacement.
 * This satisfies the determinism rule (Rule 5 of TACHYON_ENGINEERING_RULES).
 * 
 * No global state. The seed is stored in the struct.
 */
struct CameraShake {
    uint32_t seed{0};                    ///< Deterministic noise seed.
    float amplitude_position{0.0f};      ///< Max position displacement in world units.
    float amplitude_rotation{0.0f};    ///< Max rotation displacement in degrees.
    float frequency{1.0f};               ///< Noise frequency in Hz.
    float roughness{0.5f};               ///< Noise roughness (0 = smooth, 1 = jittery).
    
    /**
     * @brief Evaluate position displacement at time t.
     * 
     * Uses a simple multi-octave value noise for determinism.
     * The result is a Vector3 where each component is in [-amplitude, +amplitude].
     */
    [[nodiscard]] math::Vector3 evaluate_position(float t) const;
    
    /**
     * @brief Evaluate rotation displacement at time t.
     * 
     * Returns Euler angle offsets in degrees, each in [-amplitude, +amplitude].
     */
    [[nodiscard]] math::Vector3 evaluate_rotation(float t) const;
    
    /**
     * @brief Check if shake has any effect.
     */
    [[nodiscard]] bool is_active() const noexcept {
        return amplitude_position > 0.0f || amplitude_rotation > 0.0f;
    }
};

} // namespace tachyon::camera
