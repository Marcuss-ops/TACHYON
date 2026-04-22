#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/scene/state/evaluated_state.h"

#include <cmath>
#include <string>

namespace tachyon {
namespace scene {

constexpr double kPi = 3.141592653589793238462643383279502884;

namespace {

template <typename T>
T lerp(const T& a, const T& b, float t) {
    return a * (1.0f - t) + b * t;
}

template <typename T>
T sample_bezier_spatial(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
    const float it = 1.0f - t;
    return p0 * (it * it * it) + p1 * (3.0f * it * it * t) + p2 * (3.0f * it * t * t) + p3 * (t * t * t);
}

} // namespace

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

math::Vector2 fallback_position(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.position_x.value_or(0.0)),
        static_cast<float>(layer.transform.position_y.value_or(0.0))
    };
}

double fallback_rotation(const LayerSpec& layer) {
    return layer.transform.rotation.value_or(0.0);
}

math::Vector2 fallback_scale(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.scale_x.value_or(1.0)),
        static_cast<float>(layer.transform.scale_y.value_or(1.0))
    };
}

double sample_audio_band(const ::tachyon::audio::AudioBands& bands, int band) {
    // This was moved to audio_sampling module, but keeping here for compatibility
    switch (static_cast<AudioBandType>(band)) {
        case AudioBandType::Bass: return bands.bass;
        case AudioBandType::Mid: return bands.mid;
        case AudioBandType::High: return bands.high;
        case AudioBandType::Presence: return bands.presence;
        case AudioBandType::Rms: return bands.rms;
        default: break;
    }
    return bands.rms;
}

math::Transform2 make_transform2(
    const math::Vector2& position,
    double rotation_degrees,
    const math::Vector2& scale) {
    math::Transform2 transform;
    transform.position = position;
    transform.rotation_rad = static_cast<float>(degrees_to_radians(rotation_degrees));
    transform.scale = scale;
    return transform;
}

} // namespace scene
} // namespace tachyon
