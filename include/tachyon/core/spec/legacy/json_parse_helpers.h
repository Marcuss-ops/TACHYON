#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "tachyon/core/json/json_reader.h"
#include "tachyon/core/spec/parsing/spec_value_parsers.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/animation/easing_parse.h"

namespace tachyon {

using json = nlohmann::json;

// --- Legacy Adapters (prefer spec_value_parsers equivalents) ---
inline bool read_vector2_like(const json& value, math::Vector2& out) { return parse_vector2_value(value, out); }
inline bool read_vector2_object(const json& value, math::Vector2& out) { return parse_vector2_value(value, out); }
inline bool read_vector3_like(const json& value, math::Vector3& out) { return parse_vector3_value(value, out); }
inline bool read_vector3_object(const json& value, math::Vector3& out) { return parse_vector3_value(value, out); }

// --- Animation Parsers (Legacy aliases) ---
inline animation::EasingPreset parse_easing_preset(const json& value) { return animation::parse_easing_preset(value); }
inline animation::CubicBezierEasing parse_bezier(const json& value) { return animation::parse_bezier(value); }

} // namespace tachyon
