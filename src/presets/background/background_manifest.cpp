#include "tachyon/presets/background/background_manifest.h"
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

    for (const auto* desc : BackgroundRegistry::instance().list_all()) {
        if (desc) {
            descriptors.push_back(*desc);
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
        {"tachyon.backgroundpreset.galaxy_premium", "Galaxy Premium", "High-end cinematic galaxy background", "Premium", {"space", "premium"}, 1, registry::Stability::Stable, {}},
        {},
        [](const registry::ParameterBag& bag) {
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
        {"tachyon.backgroundpreset.dark_tech_grid", "Dark Tech Grid", "Modern dark tech grid for presentations", "Tech", {"grid", "dark"}, 1, registry::Stability::Stable, {}},
        {},
        [](const registry::ParameterBag& bag) {
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
        {"tachyon.backgroundpreset.cinematic_aura", "Cinematic Aura", "Soft cinematic aura background", "Cinematic", {"aura", "soft"}, 1, registry::Stability::Stable, {}},
        {},
        [](const registry::ParameterBag& bag) {
            BackgroundParams p = get_base_params(bag);
            p.kind = "tachyon.background.kind.aura";
            p.palette = "neon_night";
            p.softness = 0.5;
            return build_background(p);
        }
    });

    return specs;
}

std::vector<BackgroundKindSpec> BackgroundManifest::generate_kind_specs() const {
    std::vector<BackgroundKindSpec> specs;

    using namespace background::procedural_bg;

    // Helper to extract common procedural params from bag
    auto get_procedural_params = [](const registry::ParameterBag& bag) {
        ProceduralParams p;
        if (auto pa = bag.get<ColorSpec>("palette.a")) p.palette_a = *pa;
        if (auto pb = bag.get<ColorSpec>("palette.b")) p.palette_b = *pb;
        if (auto pc = bag.get<ColorSpec>("palette.c")) p.palette_c = *pc;
        p.motion_speed = bag.get_or<double>("motion_speed", 1.0);
        p.contrast = bag.get_or<double>("contrast", 1.0);
        p.grain = bag.get_or<double>("grain", 0.0);
        p.softness = bag.get_or<double>("softness", 0.0);
        p.seed = static_cast<uint64_t>(bag.get_or<int>("seed", 0));
        return p;
    };

    // Galaxy Kind
    specs.push_back({
        "tachyon.background.kind.galaxy",
        {"tachyon.background.kind.galaxy", "Galaxy", "Cinematic galaxy background type", "Core", {}, 1, registry::Stability::Stable, {}},
        {},
        [get_procedural_params](const registry::ParameterBag& bag) {
            auto p = get_procedural_params(bag);
            auto spec = make_galaxy_spec(p);
            if (auto val = bag.get<float>("star_speed")) spec.star_speed = *val;
            if (auto val = bag.get<float>("density")) spec.density = *val;
            return spec;
        }
    });

    // Grid Modern Kind
    specs.push_back({
        "tachyon.background.kind.grid_modern",
        {"tachyon.background.kind.grid_modern", "Modern Grid", "Tech grid background type", "Core", {}, 1, registry::Stability::Stable, {}},
        {},
        [get_procedural_params](const registry::ParameterBag& bag) {
            auto p = get_procedural_params(bag);
            float spacing = bag.get_or<float>("spacing", 60.0f);
            return make_modern_tech_grid_spec(p, spacing);
        }
    });

    // Aura Kind
    specs.push_back({
        "tachyon.background.kind.aura",
        {"tachyon.background.kind.aura", "Aura", "Soft aura background type", "Core", {}, 1, registry::Stability::Stable, {}},
        {},
        [get_procedural_params](const registry::ParameterBag& bag) {
            auto p = get_procedural_params(bag);
            return make_aura_spec(p);
        }
    });

    return specs;
}

} // namespace tachyon::presets
