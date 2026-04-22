#pragma once

#include "tachyon/core/animation/keyframe.h"
#include "tachyon/core/animation/bezier_solver.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace tachyon::animation {

template <typename T>
struct Interpolator {
    static T lerp(const T& a, const T& b, double t) {
        return a * static_cast<float>(1.0 - t) + b * static_cast<float>(t);
    }
};

template <>
struct Interpolator<double> {
    static double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }
};

template <>
struct Interpolator<float> {
    static float lerp(float a, float b, double t) {
        return a + (b - a) * static_cast<float>(t);
    }
};

/**
 * @brief Generic track of keyframes with temporal Bezier solving and tangents.
 */
template <typename T>
class KeyframeTrack {
public:
    using KeyframeT = Keyframe<T>;

    std::vector<KeyframeT> keyframes;

    void add_keyframe(const KeyframeT& kf) {
        keyframes.push_back(kf);
        m_sorted = false;
    }

    void sort() {
        if (!m_sorted) {
            std::sort(keyframes.begin(), keyframes.end(),
                      [](const KeyframeT& a, const KeyframeT& b) {
                          return a.time < b.time;
                      });
            m_sorted = true;
        }
    }

    [[nodiscard]] bool empty() const { return keyframes.empty(); }
    [[nodiscard]] std::size_t size() const { return keyframes.size(); }

    [[nodiscard]] T evaluate(double time) const {
        if (keyframes.empty()) return T{};

        if (keyframes.size() == 1 || time <= keyframes.front().time)
            return keyframes.front().value;
        if (time >= keyframes.back().time)
            return keyframes.back().value;

        // Binary search: find first key with time > requested time
        const auto it = std::upper_bound(
            keyframes.begin(), keyframes.end(), time,
            [](double t, const KeyframeT& kf) { return t < kf.time; });

        const KeyframeT& k0 = *(it - 1);
        const KeyframeT& k1 = *it;

        if (k0.out_mode == InterpolationMode::Hold)
            return k0.value;

        const double seg_len = k1.time - k0.time;
        double raw_t = (seg_len > 0.0) ? (time - k0.time) / seg_len : 0.0;

        double progress = raw_t;

        if (k0.out_mode == InterpolationMode::Bezier) {
            // Bezier temporal interpolation
            // Assuming tangent times are normalized in [0, 1] relative to segment length.
            double cp1_x = k0.out_tangent_time;
            double cp1_y = 0.0; // In standard AE these affect value interpolation or just temporal easing.
            // For pure temporal easing (e.g. speed graph), we solve bezier for T.
            // For now, we assume simple temporal easing using bezier control points (x1, y1, x2, y2).
            double x1 = k0.bezier.cx1;
            double y1 = k0.bezier.cy1;
            double x2 = k0.bezier.cx2;
            double y2 = k0.bezier.cy2;
            
            progress = BezierSolver::solve(raw_t, x1, y1, x2, y2);
        } else if (k0.out_mode == InterpolationMode::Linear) {
            // Apply standard easing preset
            progress = apply_easing(raw_t, k0.easing, k0.bezier);
        }

        // Apply spatial/value interpolation based on progress
        if (k0.out_mode == InterpolationMode::Bezier) {
            // Spatial bezier involves evaluating the value hermite spline if tangents are provided.
            // In many properties, it's just lerp(progress) if no spatial tangent. 
            // Fallback to lerp for this implementation unless Hermite is implemented.
            return hermite_interp(k0, k1, progress);
        }

        return Interpolator<T>::lerp(k0.value, k1.value, progress);
    }

private:
    mutable bool m_sorted{false};

    [[nodiscard]] T hermite_interp(const KeyframeT& k0, const KeyframeT& k1, double t) const {
        const double t2 = t * t;
        const double t3 = t2 * t;
        
        const double h00 =  2.0 * t3 - 3.0 * t2 + 1.0;
        const double h10 =        t3 - 2.0 * t2 + t;
        const double h01 = -2.0 * t3 + 3.0 * t2;
        const double h11 =        t3 -        t2;

        const double seg_dt = k1.time - k0.time;
        const float inv_dt = static_cast<float>(seg_dt > 0.0 ? 1.0 / seg_dt : 0.0);
        
        // Out tangent of k0, In tangent of k1 (assumed relative value deltas)
        T m0 = k0.out_tangent_value * inv_dt;
        T m1 = k1.in_tangent_value  * inv_dt;

        return k0.value * static_cast<float>(h00) + 
               m0 * static_cast<float>(h10) + 
               k1.value * static_cast<float>(h01) + 
               m1 * static_cast<float>(h11);
    }
};

} // namespace tachyon::animation
