#include "tachyon/presets/background/background_manifest.h"
#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/background_registry.h"
#include <vector>

namespace tachyon::presets {

namespace {

// Helper to get base background params
BackgroundParams get_base_params(const registry::ParameterBag& bag) {
    BackgroundParams p;
    p.w = bag.get_or<float>("width", 1920.0f);
    p.h = bag.get_or<float>("height", 1080.0f);
    p.out_point = bag.get_or<double>("duration", 8.0);
    return p;
}

} // namespace

std::vector<BackgroundDescriptor> BackgroundManifest::generate_descriptors() const {
    std::vector<BackgroundDescriptor> descriptors;
    
    // Generate descriptors from BackgroundKindRegistry
    auto kind_ids = BackgroundKindRegistry::instance().list_ids();
    for (const auto& id : kind_ids) {
        if (const auto* kind_spec = BackgroundKindRegistry::instance().find(id)) {
            BackgroundDescriptor desc;
            desc.id = kind_spec->id;
            desc.metadata = kind_spec->metadata;
            descriptors.push_back(std::move(desc));
        }
    }
    
    return descriptors;
}

std::vector<BackgroundPresetSpec> BackgroundManifest::generate_preset_specs() const {
    std::vector<BackgroundPresetSpec> specs;
    
    using namespace background::procedural_bg;
    
    // Galaxy Premium
    specs.push_back({
        "tachyon.backgroundpreset.galaxy_premium",
        {"tachyon.backgroundpreset.galaxy_premium", "Galaxy Premium", "High-end cinematic galaxy background", "Premium", {"space", "premium"}},
        {},
        [get_base_params](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.galaxy";
            p.palette = "cosmic_nebula";
            p.speed = 1.0f;
            return build_background(p);
        }
    });
    
    // Dark Tech Grid
    specs.push_back({
        "tachyon.backgroundpreset.dark_tech_grid",
        {"tachyon.backgroundpreset.dark_tech_grid", "Dark Tech Grid", "Modern dark tech grid for presentations", "Tech", {"grid", "dark"}},
        {},
        [get_base_params](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.grid_modern";
            p.palette = "dark_tech";
            p.spacing = 80.0;
            return build_background(p);
        }
    });
    
    // Cinematic Aura
    specs.push_back({
        "tachyon.backgroundpreset.cinematic_aura",
        {"tachyon.backgroundpreset.cinematic_aura", "Cinematic Aura", "Soft cinematic aura background", "Cinematic", {"aura", "soft"}},
        {},
        [get_base_params](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.aura";
            p.palette = "neon_night";
            p.softness = 0.5;
            return build_background(p);
        }
    });
    
    return specs;
}

} // namespace tachyon::presets
