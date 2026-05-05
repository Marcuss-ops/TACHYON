#pragma once

#include "tachyon/presets/preset_base.h"
#include "tachyon/presets/background/background_params.h"
#include <optional>
#include <string>
#include <vector>

namespace tachyon::presets {

struct SceneLayerEntry {
    std::string type; // "text", "image", "background"
    std::string content; // text content or asset path
    std::string style; // preset style name
};

struct EnhancedSceneParams : SceneParams {
    std::vector<SceneLayerEntry> layers;
    std::string main_text;
    std::string background_style{};
    std::string background_palette{"neon_night"};
    std::vector<AssetSpec> assets;
    std::optional<std::string> main_asset_id;
    std::optional<BackgroundParams> background;
};

} // namespace tachyon::presets
