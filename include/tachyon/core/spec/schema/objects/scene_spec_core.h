#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <filesystem>
#include <string>
#include <vector>

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/renderer2d/raster/path/path_types.h"

namespace tachyon {

using json = nlohmann::json;

// --- Utility Helpers ---
bool is_version_like(const std::string& version);
std::string make_path(const std::string& parent, const std::string& child);

// --- Basic JSON Readers ---
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
    if (!object.contains(key) || object.at(key).is_null()) {
        out = std::nullopt;
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<T>();
    return true;
}

bool read_string(const json& object, const char* key, std::string& out);
bool read_bool(const json& object, const char* key, bool& out);
bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out);

// --- Math & Type Parsers ---
bool parse_vector2_value(const json& value, math::Vector2& out);
bool parse_vector3_value(const json& value, math::Vector3& out);
bool parse_color_value(const json& value, ColorSpec& out);
bool parse_gradient_spec(const json& object, GradientSpec& out);
animation::EasingPreset parse_easing_preset(const json& value);
animation::CubicBezierEasing parse_bezier(const json& value);
renderer2d::LineCap parse_line_cap(const json& value);
renderer2d::LineJoin parse_line_join(const json& value);
std::optional<AudioBandType> parse_audio_band_type(const json& value);

// --- Keyframe Parsers ---
bool parse_scalar_keyframe(const json& object, ScalarKeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics);
bool parse_vector2_keyframe(const json& object, Vector2KeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics);
bool parse_vector3_keyframe(const json& object, AnimatedVector3Spec::Keyframe& keyframe, const std::string& path, DiagnosticBag& diagnostics);
bool parse_color_keyframe(const json& object, ColorKeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics);

// --- Property Parsers ---
void parse_optional_scalar_property(const json& object, const char* key, AnimatedScalarSpec& property, const std::string& path, DiagnosticBag& diagnostics);
void parse_optional_vector_property(const json& object, const char* key, AnimatedVector2Spec& property, const std::string& path, DiagnosticBag& diagnostics);
void parse_optional_vector3_property(const json& object, const char* key, AnimatedVector3Spec& property, const std::string& path, DiagnosticBag& diagnostics);
void parse_optional_color_property(const json& object, const char* key, AnimatedColorSpec& property, const std::string& path, DiagnosticBag& diagnostics);

// --- Structural Parsers ---
void parse_transform(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_shape_path(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
LayerSpec parse_layer(const json& object, const std::string& path, DiagnosticBag& diagnostics);
CompositionSpec parse_composition(const json& object, const std::string& path, DiagnosticBag& diagnostics);
AssetSpec parse_asset(const json& object, const std::string& path, DiagnosticBag& diagnostics);
ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path);
ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text);
ValidationResult validate_scene_spec(const SceneSpec& scene);

// --- Serialization ---
json serialize_scene_spec(const SceneSpec& scene);
json serialize_composition(const CompositionSpec& comp);
json serialize_layer(const LayerSpec& layer);

} // namespace tachyon
