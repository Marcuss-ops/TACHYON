#include "tachyon/tracker/optical_flow_helpers.h"
#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace tachyon::tracker {

TranslationEstimate estimate_translation(
    const std::vector<float>& prev,
    const std::vector<float>& curr,
    int width,
    int height,
    int max_shift) {

    TranslationEstimate estimate;
    if (width <= 0 || height <= 0) {
        return estimate;
    }

    const int shift_limit = std::max(0, std::min(max_shift, std::min(width - 1, height - 1)));
    if (shift_limit == 0) {
        return estimate;
    }

    auto sample = [&](const std::vector<float>& frame, int x, int y) -> float {
        return frame[static_cast<std::size_t>(y * width + x)];
    };

    float baseline_mse = std::numeric_limits<float>::infinity();
    float best_mse = std::numeric_limits<float>::infinity();
    int best_dx = 0;
    int best_dy = 0;

    for (int dy = -shift_limit; dy <= shift_limit; ++dy) {
        for (int dx = -shift_limit; dx <= shift_limit; ++dx) {
            float error = 0.0f;
            int samples = 0;

            for (int y = 0; y < height; ++y) {
                const int py = y - dy;
                if (py < 0 || py >= height) {
                    continue;
                }
                for (int x = 0; x < width; ++x) {
                    const int px = x - dx;
                    if (px < 0 || px >= width) {
                        continue;
                    }

                    const float delta = sample(prev, px, py) - sample(curr, x, y);
                    error += delta * delta;
                    ++samples;
                }
            }

            if (samples == 0) {
                continue;
            }

            const float mse = error / static_cast<float>(samples);
            if (dx == 0 && dy == 0) {
                baseline_mse = mse;
            }

            if (mse < best_mse) {
                best_mse = mse;
                best_dx = dx;
                best_dy = dy;
            }
        }
    }

    if (!std::isfinite(baseline_mse) || !std::isfinite(best_mse)) {
        return estimate;
    }

    const float improvement = (baseline_mse - best_mse) / std::max(1e-6f, baseline_mse);
    if (improvement < 0.05f) {
        return estimate;
    }

    estimate.found = true;
    estimate.dx = best_dx;
    estimate.dy = best_dy;
    estimate.confidence = std::clamp(improvement, 0.0f, 1.0f);
    if (std::abs(best_dx) == shift_limit || std::abs(best_dy) == shift_limit) {
        estimate.confidence = std::min(estimate.confidence, 0.25f);
    } else {
        estimate.confidence = std::max(estimate.confidence, 0.65f);
    }

    return estimate;
}

void apply_translation_estimate(
    OpticalFlowResult& result,
    const TranslationEstimate& estimate) {

    if (!estimate.found) {
        return;
    }

    for (auto& vec : result.vectors) {
        vec.flow = math::Vector2(static_cast<float>(estimate.dx), static_cast<float>(estimate.dy));
        vec.confidence = estimate.confidence;
        vec.valid = estimate.confidence > 0.0f;
        vec.age = 0;
        vec.source_level = -1;
    }
}

} // namespace tachyon::tracker
