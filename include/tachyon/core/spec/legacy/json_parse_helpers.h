#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

namespace tachyon {

using json = nlohmann::json;

// --- Basic JSON Readers ---
bool read_string(const json& object, const char* key, std::string& out);
bool read_bool(const json& object, const char* key, bool& out);
bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out);

template <typename T>
bool read_number(const json& object, const char* key, T& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<T>();
    return true;
}

template <typename T>
bool read_number(const json& object, const char* key, std::optional<T>& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<T>();
    return true;
}

// --- Math Parsers ---
bool read_vector2_like(const json& value, math::Vector2& out);
bool read_vector2_object(const json& value, math::Vector2& out);
bool parse_vector2_value(const json& value, math::Vector2& out);
bool read_vector3_like(const json& value, math::Vector3& out);
bool read_vector3_object(const json& value, math::Vector3& out);
bool parse_vector3_value(const json& value, math::Vector3& out);
bool parse_color_value(const json& value, ColorSpec& out);
bool parse_gradient_spec(const json& object, GradientSpec& out);

// --- Enum Parsers ---
animation::EasingPreset parse_easing_preset(const json& value);
animation::CubicBezierEasing parse_bezier(const json& value);
renderer2d::LineCap parse_line_cap(const json& value);
renderer2d::LineJoin parse_line_join(const json& value);
std::optional<AudioBandType> parse_audio_band_type(const json& value);

} // namespace tachyon
