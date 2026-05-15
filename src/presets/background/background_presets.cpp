#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/catalog/presets/preset_catalog.h"
#include "tachyon/catalog/presets/preset_entry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::presets {

namespace {

BackgroundParams get_base_params(const registry::ParameterBag& bag) {
    BackgroundParams p;
    p.w = bag.get_or<float>("width", 1920.0f);
    p.h = bag.get_or<float>("height", 1080.0f);
    p.out_point = bag.get_or<double>("duration", 8.0);
    return p;
}

} // namespace

void register_background_presets(content::PresetCatalog& catalog) {
    using namespace background::procedural_bg;

    // Galaxy Premium
    {
        content::PresetEntry entry;
        entry.id = "tachyon.backgroundpreset.galaxy_premium";
        entry.kind = content::ContentKind::Background;
        entry.metadata = {"tachyon.backgroundpreset.galaxy_premium", "Galaxy Premium", "High-end cinematic galaxy background", "Premium", {"space", "premium"}, 1, registry::Stability::Stable, {}};
        catalog.register_entry(std::move(entry));

        catalog.register_background("tachyon.backgroundpreset.galaxy_premium", [](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.galaxy";
            p.palette = "cosmic_nebula";
            p.speed = 1.0f;
            return build_background(p);
        });
    }
    
    // Dark Tech Grid
    {
        content::PresetEntry entry;
        entry.id = "tachyon.backgroundpreset.dark_tech_grid";
        entry.kind = content::ContentKind::Background;
        entry.metadata = {"tachyon.backgroundpreset.dark_tech_grid", "Dark Tech Grid", "Modern dark tech grid for presentations", "Tech", {"grid", "dark"}, 1, registry::Stability::Stable, {}};
        catalog.register_entry(std::move(entry));

        catalog.register_background("tachyon.backgroundpreset.dark_tech_grid", [](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.grid_modern";
            p.palette = "dark_tech";
            p.spacing = 80.0;
            return build_background(p);
        });
    }

    // Cinematic Aura
    {
        content::PresetEntry entry;
        entry.id = "tachyon.backgroundpreset.cinematic_aura";
        entry.kind = content::ContentKind::Background;
        entry.metadata = {"tachyon.backgroundpreset.cinematic_aura", "Cinematic Aura", "Soft cinematic aura background", "Cinematic", {"aura", "soft"}, 1, registry::Stability::Stable, {}};
        catalog.register_entry(std::move(entry));

        catalog.register_background("tachyon.backgroundpreset.cinematic_aura", [](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.aura";
            p.palette = "neon_night";
            p.softness = 0.5;
            return build_background(p);
        });
    }
}

} // namespace tachyon::presets
