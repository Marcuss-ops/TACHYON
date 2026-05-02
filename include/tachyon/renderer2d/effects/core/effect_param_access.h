#pragma once

#include "tachyon/renderer2d/effects/core/effect_params.h"
#include <string>
#include <initializer_list>

namespace tachyon::renderer2d {

/**
 * @brief Checks if any of the provided keys exist as scalar parameters.
 */
bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys);

/**
 * @brief Checks if any of the provided keys exist as color parameters.
 */
bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys);

/**
 * @brief Retrieves a scalar parameter by key, with fallback.
 */
float get_scalar(const EffectParams& params, const std::string& key, float fallback);

/**
 * @brief Retrieves a color parameter by key, with fallback.
 */
Color get_color(const EffectParams& params, const std::string& key, Color fallback);

/**
 * @brief Retrieves a string parameter by key, with fallback.
 */
std::string get_string(const EffectParams& params, const std::string& key, const std::string& fallback);

} // namespace tachyon::renderer2d
