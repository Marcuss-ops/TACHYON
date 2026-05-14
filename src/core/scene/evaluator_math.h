#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/timeline/time.h"
#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/audio/audio_interfaces.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

namespace tachyon {
namespace scene {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

double degrees_to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

math::Vector2 lerp(const math::Vector2& a, const math::Vector2& b, float t) {
    return a * (1.0f - t) + b * t;
}

} // namespace

math::Vector2 fallback_position(const class LayerSpec& layer);
math::Vector2 fallback_scale(const class LayerSpec& layer);
double fallback_rotation(const class LayerSpec& layer);
double sample_audio_band(const ::tachyon::audio::AudioBands& bands, int band);

} // namespace scene
} // namespace tachyon