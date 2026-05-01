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
#include "tachyon/core/spec/legacy/json_parse_helpers.h"

namespace tachyon {

using json = nlohmann::json;

// --- Utility Helpers ---
std::string make_path(const std::string& parent, const std::string& child);
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

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
void parse_layer(const json& object, LayerSpec& out, const std::string& path, DiagnosticBag& diagnostics);
CompositionSpec parse_composition(const json& object, const std::string& path, DiagnosticBag& diagnostics);
AssetSpec parse_asset(const json& object, const std::string& path, DiagnosticBag& diagnostics);
ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path);
ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text, const std::filesystem::path& base_dir = {});
ValidationResult validate_scene_spec(const SceneSpec& scene);

// --- Serialization ---
json serialize_scene_spec(const SceneSpec& scene);
json serialize_composition(const CompositionSpec& comp);
json serialize_layer(const LayerSpec& layer);

} // namespace tachyon
