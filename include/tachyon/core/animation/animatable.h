#pragma once

#include <vector>
#include <algorithm>

namespace tachyon {
namespace animation {

/**
 * Supported interpolation modes for keyframes.
 */
enum class InterpolationMode {
    Hold,
    Linear,
    Bezier
};

/**
 * A single keyframe in time.
 */
template <typename T>
struct Keyframe {
    double time;
    T value;
    InterpolationMode in_mode{InterpolationMode::Linear};
    InterpolationMode out_mode{InterpolationMode::Linear};
    
    // For Bezier, we would add control points/tensions here.
};

/**
 * Baseline for a value that is animated over time using keyframes.
 */
template <typename T>
struct AnimationCurve {
    std::vector<Keyframe<T>> keyframes;

    [[nodiscard]] bool empty() const { return keyframes.empty(); }

    [[nodiscard]] T evaluate(double time) const {
        if (keyframes.empty()) return T{};
        if (keyframes.size() == 1 || time <= keyframes.front().time) {
            return keyframes.front().value;
        }
        if (time >= keyframes.back().time) {
            return keyframes.back().value;
        }

        // Find the segment
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time, 
            [](const Keyframe<T>& kb, double t) {
                return kb.time < t;
            });

        const auto& next = *it;
        const auto& prev = *(it - 1);

        if (prev.out_mode == InterpolationMode::Hold) {
            return prev.value;
        }

        // Linear (simplified)
        double t = (time - prev.time) / (next.time - prev.time);
        
        // This assumes T supports operator+, operator* (like Vector3, float)
        // In a real engine, we'd use a generic lerp traits.
        return prev.value * static_cast<float>(1.0 - t) + next.value * static_cast<float>(t);
    }
};

} // namespace animation
} // namespace tachyon
