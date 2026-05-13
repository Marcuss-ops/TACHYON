#pragma once

#include <optional>
#include <filesystem>
#include <string>
#include <vector>

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon {

// --- Utility Helpers ---
TACHYON_API std::string make_path(const std::string& parent, const std::string& child);
TACHYON_API std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

// --- Validation ---
ValidationResult validate_scene_spec(const SceneSpec& scene);

} // namespace tachyon
