#pragma once

#include <optional>
#include <filesystem>
#include <string>
#include <vector>

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/types/diagnostics.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/spec/schema/common/gradient_spec.h"

namespace tachyon {

// --- Utility Helpers ---
std::string make_path(const std::string& parent, const std::string& child);
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

// --- Validation ---
ValidationResult validate_scene_spec(const SceneSpec& scene);

} // namespace tachyon
