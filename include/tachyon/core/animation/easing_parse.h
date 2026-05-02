#pragma once

#include "tachyon/core/animation/easing.h"
#include <string>
#include <nlohmann/json.hpp>

namespace tachyon::animation {

/**
 * @brief Parses an easing preset from a string.
 */
EasingPreset parse_easing_preset(const std::string& name);

/**
 * @brief Parses an easing preset from JSON.
 */
EasingPreset parse_easing_preset(const nlohmann::json& j);

/**
 * @brief Parses a bezier curve from JSON.
 */
bool parse_bezier(const nlohmann::json& j, float& x1, float& y1, float& x2, float& y2);

/**
 * @brief Legacy adapter for bezier parsing.
 */
CubicBezierEasing parse_bezier(const nlohmann::json& j);

} // namespace tachyon::animation
