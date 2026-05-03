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
#include "tachyon/core/json/json_reader.h"

namespace tachyon {

using json = nlohmann::json;

// --- Utility Helpers ---
std::string make_path(const std::string& parent, const std::string& child);
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

// --- Validation ---
ValidationResult validate_scene_spec(const SceneSpec& scene);

// --- Serialization ---
json serialize_scene_spec(const SceneSpec& scene);
json serialize_composition(const CompositionSpec& comp);
json serialize_layer(const LayerSpec& layer);

} // namespace tachyon
