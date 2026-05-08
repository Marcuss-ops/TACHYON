#include "tachyon/backgrounds.hpp"
#include "tachyon/background_registry.h"
#include "tachyon/background_descriptor.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/parameter_bag.h"

namespace tachyon {

void BackgroundRegistry::register_all_builtins() {
    // 1. Basic backgrounds
    register_descriptor(make_solid_background_descriptor());
    register_descriptor(make_gradient_background_descriptor());
    register_descriptor(make_radial_gradient_background_descriptor());
    register_descriptor(make_image_background_descriptor());
    register_descriptor(make_video_background_descriptor());

    // 2. Procedural backgrounds (advanced)
    using namespace presets::background::procedural_bg;
    using namespace registry;

    auto get_procedural_params = [](const ParameterBag& bag) {
        ProceduralParams p;
        p.palette_a = bag.get_or<ColorSpec>("palette.a", ColorSpec{0,0,0,255});
        p.palette_b = bag.get_or<ColorSpec>("palette.b", ColorSpec{255,255,255,255});
        p.palette_c = bag.get_or<ColorSpec>("palette.c", ColorSpec{0,0,0,0});
        p.motion_speed = bag.get_or<double>("motion_speed", 1.0);
        p.contrast = bag.get_or<double>("contrast", 1.0);
        p.grain = bag.get_or<double>("grain", 0.0);
        p.softness = bag.get_or<double>("softness", 0.0);
        p.seed = static_cast<uint64_t>(bag.get_or<int>("seed", 0));
        return p;
    };

    auto make_procedural_builder = [get_procedural_params](auto make_spec_fn) {
        return [get_procedural_params, make_spec_fn](const ParameterBag& bag) -> LayerSpec {
            LayerSpec layer;
            layer.type = LayerType::Procedural;
            layer.procedural = make_spec_fn(get_procedural_params(bag));
            return layer;
        };
    };

    // Aura
    {
        BackgroundDescriptor desc;
        desc.id = "tachyon.background.kind.aura";
        desc.aliases = {"aura", "Aura"};
        desc.kind = BackgroundKind::Procedural;
        desc.params = ParameterSchema({
            ParameterDef("motion_speed", "Speed", "Animation speed", 1.0, 0.0, 10.0),
            ParameterDef("palette.a", "Color A", "Primary color", ColorSpec{10, 10, 20, 255}),
            ParameterDef("palette.b", "Color B", "Secondary color", ColorSpec{50, 20, 80, 255})
        });
        desc.build = make_procedural_builder([](const ProceduralParams& p) { return make_aura_spec(p); });
        register_descriptor(desc);
    }

    // Modern Grid
    {
        BackgroundDescriptor desc;
        desc.id = "tachyon.background.kind.grid_modern";
        desc.kind = BackgroundKind::Procedural;
        desc.build = make_procedural_builder([](const ProceduralParams& p) { return make_modern_tech_grid_spec(p); });
        register_descriptor(desc);
    }

    // Stars
    {
        BackgroundDescriptor desc;
        desc.id = "tachyon.background.kind.stars";
        desc.kind = BackgroundKind::Procedural;
        desc.build = make_procedural_builder([](const ProceduralParams& p) { return make_stars_spec(p); });
        register_descriptor(desc);
    }
}

} // namespace tachyon
