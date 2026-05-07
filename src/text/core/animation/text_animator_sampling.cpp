#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/animation/property_sampler.h"

namespace tachyon::text {

double sample_scalar_kfs(
    const std::optional<double>& static_val,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    float t) {
    return animation::sample_scalar(static_val, keyframes, static_cast<double>(t), 0.0);
}

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t) {
    return animation::sample_color(static_val, keyframes, static_cast<double>(t), ::tachyon::ColorSpec{255, 255, 255, 255});
}

math::Vector2 sample_vector2_kfs(
    const std::optional<math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t) {
    return animation::sample_vector2(static_val, keyframes, static_cast<double>(t), math::Vector2{0.0f, 0.0f});
}

} // namespace tachyon::text

