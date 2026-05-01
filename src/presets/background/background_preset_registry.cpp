#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/text/text_builders.h"

namespace tachyon::presets {

std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height) {
    using namespace background;
    
    if (id == "midnight_silk")        return procedural_bg::midnight_silk(width, height);
    if (id == "golden_horizon")       return procedural_bg::golden_horizon(width, height);
    if (id == "cyber_matrix")         return procedural_bg::cyber_matrix(width, height);
    if (id == "frosted_glass")        return procedural_bg::frosted_glass(width, height);
    if (id == "cosmic_nebula")        return procedural_bg::cosmic_nebula(width, height);
    if (id == "brushed_metal")        return procedural_bg::brushed_metal(width, height);
    if (id == "oceanic_abyss")        return procedural_bg::oceanic_abyss(width, height);
    if (id == "royal_velvet")         return procedural_bg::royal_velvet(width, height);
    if (id == "prismatic_light")      return procedural_bg::prismatic_light(width, height);
    if (id == "technical_blueprint")  return procedural_bg::technical_blueprint(width, height);
    
    if (id == "brushed_metal_title") {
        presets::TextParams tp;
        tp.text = "TACHYON NATIVE";
        tp.font_size = 120;
        tp.reveal_duration = 0.8;
        return presets::build_text_brushed_metal_title(tp);
    }

    return std::nullopt;
}

std::vector<std::string> list_background_presets() {
    return {
        "midnight_silk", "golden_horizon", "cyber_matrix", "frosted_glass",
        "cosmic_nebula", "brushed_metal", "oceanic_abyss", "royal_velvet",
        "prismatic_light", "technical_blueprint", "brushed_metal_title"
    };
}

} // namespace tachyon::presets
