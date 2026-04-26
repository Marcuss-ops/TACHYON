#pragma once

#include "tachyon/runtime/core/contracts/math_contract.h"
#include "tachyon/core/animation/easing.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace tachyon {

//------------------------------------------------------------------------------
// interpolate() - Top-level interpolation (Remotion-style)
//------------------------------------------------------------------------------

/**
 * Interpolates between scalar values at specific frames with optional easing.
 *
 * Usage:
 *   float val = interpolate(frame, {0, 30}, {0.0, 1.0});
 *   float val = interpolate(frame, {0, 30}, {0.0, 1.0}, EasingPreset::EaseInOut);
 */
[[nodiscard]] inline double interpolate(double frame, const std::vector<double>& frames, const std::vector<double>& values, 
                                        animation::EasingPreset easing = animation::EasingPreset::None) {
    if (frames.empty() || values.empty() || frames.size() != values.size()) {
        return 0.0;
    }
    if (frames.size() == 1) {
        return values[0];
    }

    // Clamp to range
    if (frame <= frames.front()) return values.front();
    if (frame >= frames.back()) return values.back();

    // Find segment
    for (std::size_t i = 0; i < frames.size() - 1; ++i) {
        if (frame >= frames[i] && frame <= frames[i + 1]) {
            double t = (frame - frames[i]) / (frames[i + 1] - frames[i]);
            if (easing != animation::EasingPreset::None) {
                t = animation::apply_easing(t, easing);
            }
            return values[i] * (1.0 - t) + values[i + 1] * t;
        }
    }
    return values.back();
}

//------------------------------------------------------------------------------
// random(seed) - Deterministic random (Remotion-style)
//------------------------------------------------------------------------------

/**
 * Deterministic random based on seed string. Same seed = same value always.
 * Uses existing math_contract::splitmix64 and math_contract::noise.
 *
 * Usage:
 *   float x = random("particle-x-0");         // 0.0-1.0
 *   float y = random("particle-y-0", -100, 100); // range
 */
[[nodiscard]] inline double random(std::string_view seed) {
    std::uint64_t h = 0x517cc1b727220a95ULL;
    for (char c : seed) {
        h = (h * 0x100000001b3ULL) ^ static_cast<std::uint64_t>(static_cast<unsigned char>(c));
    }
    return math_contract::noise(h);
}

[[nodiscard]] inline float random(std::string_view seed, float min_val, float max_val) {
    const double n = random(seed);
    return static_cast<float>(min_val + n * (max_val - min_val));
}

//------------------------------------------------------------------------------
// spring() - Re-expose existing math_contract::spring as user-friendly function
//------------------------------------------------------------------------------

/**
 * Spring physics (damped harmonic oscillator).
 * Wraps existing math_contract::spring for easier use.
 *
 * Usage:
 *   float pos = spring(frame / fps, 0.0, 1.0, 2.0, 0.5);
 */
[[nodiscard]] inline double spring(double t, double from, double to, double freq, double damping) {
    return math_contract::spring(t, from, to, freq, damping);
}

//------------------------------------------------------------------------------
// Noise 2D/3D/4D - Built on existing math_contract::noise (1D)
//------------------------------------------------------------------------------

/**
 * Simplex noise 2D. For organic motion graphics.
 * Usage: float n = noise2d(x * 0.05f, y * 0.05f) * 100.0f;
 */
[[nodiscard]] float noise2d(float x, float y) noexcept;

/**
 * Simplex noise 3D.
 */
[[nodiscard]] float noise3d(float x, float y, float z) noexcept;

/**
 * Simplex noise 4D.
 */
[[nodiscard]] float noise4d(float x, float y, float z, float w) noexcept;

}  // namespace tachyon
