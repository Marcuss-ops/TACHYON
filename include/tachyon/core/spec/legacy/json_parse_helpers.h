#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "tachyon/core/json/json_reader.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

namespace tachyon {

using json = nlohmann::json;

// --- Math Parsers (legacy wrappers, prefer json_reader equivalents) ---
inline bool read_vector2_like(const json& value, math::Vector2& out) { return read_vector2(value, out); }
inline bool read_vector2_object(const json& value, math::Vector2& out) { return read_vector2(value, out); }
inline bool parse_vector2_value(const json& value, math::Vector2& out) { return read_vector2(value, out); }
inline bool read_vector3_like(const json& value, math::Vector3& out) { return read_vector3(value, out); }
inline bool read_vector3_object(const json& value, math::Vector3& out) { return read_vector3(value, out); }
inline bool parse_vector3_value(const json& value, math::Vector3& out) { return read_vector3(value, out); }
inline bool parse_color_value(const json& value, ColorSpec& out) { return read_color(value, out); }

bool parse_gradient_spec(const json& object, GradientSpec& out);

// --- Enum Parsers ---
animation::EasingPreset parse_easing_preset(const json& value);
animation::CubicBezierEasing parse_bezier(const json& value);
renderer2d::LineCap parse_line_cap(const json& value);
renderer2d::LineJoin parse_line_join(const json& value);
std::optional<AudioBandType> parse_audio_band_type(const json& value);

} // namespace tachyon
