#pragma once

#include "tachyon/core/animation/easing.h"

// Forward easing functionality to renderer2d namespace
namespace tachyon::renderer2d::animation {

using EasingPreset = tachyon::animation::EasingPreset;
using CubicBezierEasing = tachyon::animation::CubicBezierEasing;

inline double apply_easing(double t, EasingPreset preset, const CubicBezierEasing& custom = {}) {
    return tachyon::animation::apply_easing(t, preset, custom);
}

} // namespace tachyon::renderer2d::animation