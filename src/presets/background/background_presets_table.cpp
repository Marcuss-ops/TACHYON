#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/builders_common.h"

namespace tachyon::presets {

namespace {

using namespace tachyon::presets::background::procedural_bg;

void register_procedural(BackgroundPresetRegistry& registry, std::string id, std::string name, std::function<ProceduralSpec()> factory) {
    registry.register_spec({
        id,
        {id, name, "Premium procedural background.", "background.preset", {"procedural", "premium"}},
        [factory](int width, int height, double duration) {
            LayerSpec bg = make_base_layer("bg_layer", "Procedural", "procedural", {
                0.0, duration, 0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 1.0f
            });
            bg.procedural = factory();
            return bg;
        }
    });
}

} // namespace

void BackgroundPresetRegistry::load_builtins() {
    register_procedural(*this, "tachyon.background.preset.galaxy_premium", "Premium Galaxy", []() { return make_galaxy_spec(); });
    // Add more presets as needed...
}

} // namespace tachyon::presets
