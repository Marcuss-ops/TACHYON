#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/procedural.h"

namespace tachyon::presets {

void BackgroundKindRegistry::load_builtins() {
    using namespace background::procedural_bg;
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

    // 1. Aura
    this->register_spec({
        "tachyon.background.kind.aura",
        {"tachyon.background.kind.aura", "Aura", "Organic mesh gradient", "Background", {"noise", "procedural"}},
        ParameterSchema({
            ParameterDef("motion_speed", "Speed", "Animation speed", 1.0, 0.0, 10.0),
            ParameterDef("contrast", "Contrast", "Visual contrast", 1.0, 0.1, 5.0),
            ParameterDef("softness", "Softness", "Edge softness", 0.0, 0.0, 1.0),
            ParameterDef("grain", "Grain", "Film grain intensity", 0.0, 0.0, 1.0),
            ParameterDef("palette.a", "Color A", "Primary color", ColorSpec{10, 10, 20, 255}),
            ParameterDef("palette.b", "Color B", "Secondary color", ColorSpec{50, 20, 80, 255}),
            ParameterDef("seed", "Seed", "Random seed", 0)
        }),
        [get_procedural_params](const ParameterBag& bag) {
            return make_aura_spec(get_procedural_params(bag));
        }
    });

    // 2. Modern Grid
    this->register_spec({
        "tachyon.background.kind.grid_modern",
        {"tachyon.background.kind.grid_modern", "Modern Grid", "Tech-style grid with scanlines", "Background", {"grid", "tech"}},
        ParameterSchema({
            ParameterDef("spacing", "Spacing", "Grid cell size", 60.0, 10.0, 500.0),
            ParameterDef("motion_speed", "Speed", "Scroll speed", 1.0, 0.0, 10.0),
            ParameterDef("palette.a", "Background", "Grid background color", ColorSpec{5, 5, 10, 255}),
            ParameterDef("palette.b", "Lines", "Grid lines color", ColorSpec{0, 255, 128, 255})
        }),
        [get_procedural_params](const ParameterBag& bag) {
            return make_modern_tech_grid_spec(get_procedural_params(bag), bag.get_or<double>("spacing", 60.0));
        }
    });

    // 3. Stars
    this->register_spec({
        "tachyon.background.kind.stars",
        {"tachyon.background.kind.stars", "Stars", "Simple starfield", "Background", {"space", "stars"}},
        ParameterSchema({
            ParameterDef("motion_speed", "Speed", "Camera motion speed", 1.0, 0.0, 20.0),
            ParameterDef("seed", "Seed", "Random seed", 0)
        }),
        [get_procedural_params](const ParameterBag& bag) {
            return make_stars_spec(get_procedural_params(bag));
        }
    });

    // 4. Galaxy
    this->register_spec({
        "tachyon.background.kind.galaxy",
        {"tachyon.background.kind.galaxy", "Galaxy", "Advanced rotating starfield with repulsion", "Background", {"space", "galaxy"}},
        ParameterSchema({
            ParameterDef("motion_speed", "Speed", "Rotation speed", 1.0, 0.0, 10.0),
            ParameterDef("mouse_influence", "Interaction", "Mouse repulsion strength", 2.0, 0.0, 10.0),
            ParameterDef("mouse_radius", "Radius", "Mouse repulsion radius", 0.5, 0.0, 2.0)
        }),
        [get_procedural_params](const ParameterBag& bag) {
            auto spec = make_galaxy_spec(get_procedural_params(bag));
            spec.mouse_influence = static_cast<float>(bag.get_or<double>("mouse_influence", 2.0));
            spec.mouse_radius = static_cast<float>(bag.get_or<double>("mouse_radius", 0.5));
            spec.focal_x = bag.get_or<float>("mouse_x", 0.5f);
            spec.focal_y = bag.get_or<float>("mouse_y", 0.5f);
            return spec;
        }
    });

    // 5. Classic Grid (Legacy/Simple)
    this->register_spec({
        "tachyon.background.kind.grid",
        {"tachyon.background.kind.grid", "Grid", "Standard geometric grid", "Background", {"grid"}},
        ParameterSchema({
            ParameterDef("shape", "Shape", "Grid cell shape (square, circle, dot)", "square"),
            ParameterDef("spacing", "Spacing", "Cell spacing", 40.0, 4.0, 200.0),
            ParameterDef("border_width", "Width", "Line/Border width", 1.0, 0.1, 20.0)
        }),
        [get_procedural_params](const ParameterBag& bag) {
            auto p = get_procedural_params(bag);
            background::ShapeGridParams gp;
            gp.shape = bag.get_or<std::string>("shape", "square");
            gp.spacing = static_cast<float>(bag.get_or<double>("spacing", 40.0));
            gp.border_width = static_cast<float>(bag.get_or<double>("border_width", 1.0));
            gp.background_color = p.palette_a;
            gp.grid_color = p.palette_b;
            gp.speed = static_cast<float>(p.motion_speed);
            gp.seed = p.seed;
            return background::generate_shape_grid_spec(gp);
        }
    });
}

} // namespace tachyon::presets
