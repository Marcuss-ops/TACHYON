#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "tachyon/core/json/json_reader.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/types/color_spec.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/renderer2d/raster/path/path_types.h"
#include "tachyon/core/spec/schema/audio/audio_spec.h"

namespace tachyon {

using json = nlohmann::json;

// --- Domain Value Parsers ---

bool parse_vector2_value(const json& value, math::Vector2& out);
bool parse_vector3_value(const json& value, math::Vector3& out);
bool parse_color_value(const json& value, ColorSpec& out);

bool parse_gradient_spec(const json& object, GradientSpec& out);

renderer2d::LineCap parse_line_cap(const json& value);
renderer2d::LineJoin parse_line_join(const json& value);

std::optional<AudioBandType> parse_audio_band_type(const json& value);

} // namespace tachyon
