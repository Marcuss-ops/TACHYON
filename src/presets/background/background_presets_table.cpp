#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"

namespace tachyon::presets {

void BackgroundPresetRegistry::load_builtins() {
    using namespace background::procedural_bg;
    using namespace registry;

    auto get_base_params = [](const ParameterBag& bag) {
        BackgroundParams p;
        p.w = bag.get_or<float>("width", 1920.0f);
        p.h = bag.get_or<float>("height", 1080.0f);
        p.out_point = bag.get_or<double>("duration", 8.0);
        return p;
    };

    // 1. Galaxy Premium
    register_spec({
        "tachyon.backgroundpreset.galaxy_premium",
        {"tachyon.backgroundpreset.galaxy_premium", "Galaxy Premium", "High-end cinematic galaxy background", "Premium", {"space", "premium"}},
        {},
        [get_base_params](const ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.galaxy";
            p.palette = "cosmic_nebula";
            p.speed = 1.0f;
            return build_background(p);
        }
    });

    // 2. Dark Tech Grid
    register_spec({
        "tachyon.backgroundpreset.dark_tech_grid",
        {"tachyon.backgroundpreset.dark_tech_grid", "Dark Tech Grid", "Modern dark tech grid for presentations", "Tech", {"grid", "dark"}},
        {},
        [get_base_params](const ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.grid_modern";
            p.palette = "dark_tech";
            p.spacing = 80.0;
            return build_background(p);
        }
    });

    // 3. Cinematic Aura
    register_spec({
        "tachyon.backgroundpreset.cinematic_aura",
        {"tachyon.backgroundpreset.cinematic_aura", "Cinematic Aura", "Soft cinematic aura background", "Cinematic", {"aura", "soft"}},
        {},
        [get_base_params](const ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.aura";
            p.palette = "neon_night";
            p.softness = 0.5;
            return build_background(p);
        }
    });
}

} // namespace tachyon::presets
