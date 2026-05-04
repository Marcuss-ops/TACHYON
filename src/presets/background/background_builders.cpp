#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/builders_common.h"

namespace tachyon::presets {

namespace {

using namespace background::procedural_bg;

ProceduralParams get_palette(const std::string& name) {
    if (name == "dark_tech")      return palettes::dark_tech();
    if (name == "warm_amber")    return palettes::warm_amber();
    if (name == "cool_mint")      return palettes::cool_mint();
    if (name == "neon_night")     return palettes::neon_night();
    if (name == "pure_white")     return palettes::pure_white();
    if (name == "clean_white")    return palettes::clean_white();
    if (name == "warm_white")     return palettes::warm_white();
    if (name == "soft_white")     return palettes::soft_white();
    if (name == "premium_dark")   return palettes::premium_dark();
    if (name == "neon_grid")      return palettes::neon_grid();
    if (name == "midnight_silk")  return palettes::midnight_silk();
    if (name == "golden_horizon") return palettes::golden_horizon();
    if (name == "cyber_matrix")   return palettes::cyber_matrix();
    if (name == "frosted_glass")  return palettes::frosted_glass();
    if (name == "cosmic_nebula")  return palettes::cosmic_nebula();
    if (name == "brushed_metal")  return palettes::brushed_metal();
    if (name == "oceanic_abyss")  return palettes::oceanic_abyss();
    if (name == "royal_velvet")   return palettes::royal_velvet();
    if (name == "prismatic_light") return palettes::prismatic_light();
    return palettes::premium_dark();
}

} // namespace

LayerSpec build_background(const BackgroundParams& p) {
    auto palette = get_palette(p.palette);
    if (p.seed != 0) palette.seed = p.seed;
    palette.motion_speed *= static_cast<double>(p.speed);
    
    double dur = p.out_point - p.in_point;
    int w = static_cast<int>(p.w);
    int h = static_cast<int>(p.h);

    LayerSpec layer = make_base_layer("bg_layer", "Background", "procedural", {
        p.in_point, p.out_point, p.x, p.y, p.w, p.h, p.opacity
    });

    ProceduralSpec spec;
    if (p.kind == "grid_modern")    spec = make_modern_tech_grid_spec(palette, 60.0);
    else if (p.kind == "grid")      spec = make_clean_grid_spec(palette);
    else if (p.kind == "aura")      spec = make_aura_spec(palette);
    else if (p.kind == "stars")     spec = make_stars_spec(palette);
    else if (p.kind == "noise")     spec = make_noise_spec(palette);
    else if (p.kind == "texture")   spec = make_subtle_texture_spec(palette);
    else if (p.kind == "soft_gradient") spec = make_soft_gradient_spec(palette);
    else if (p.kind == "dots")      spec = make_elegant_dots_spec(palette);
    else if (p.kind == "silk")      spec = make_midnight_silk_spec(palette);
    else if (p.kind == "horizon")   spec = make_golden_horizon_spec(palette);
    else if (p.kind == "cyber")     spec = make_cyber_matrix_spec(palette);
    else if (p.kind == "glass")     spec = make_frosted_glass_spec(palette);
    else if (p.kind == "nebula")    spec = make_cosmic_nebula_spec(palette);
    else if (p.kind == "metal")     spec = make_brushed_metal_spec(palette);
    else if (p.kind == "ocean")     spec = make_oceanic_abyss_spec(palette);
    else if (p.kind == "ripple_grid") spec = make_ripple_grid_spec(palette);
    else                            spec = make_classico_premium_spec(palette);

    // Apply overrides
    if (p.frequency.has_value())    spec.frequency = *p.frequency;
    if (p.amplitude.has_value())    spec.amplitude = *p.amplitude;
    if (p.scale.has_value())        spec.scale = *p.scale;
    if (p.contrast.has_value())     spec.contrast = *p.contrast;
    if (p.softness.has_value())     spec.softness = *p.softness;
    if (p.grain_amount.has_value()) spec.grain_amount = *p.grain_amount;
    if (p.shape.has_value())        spec.shape = *p.shape;
    if (p.spacing.has_value())      spec.spacing = *p.spacing;
    if (p.border_width.has_value()) spec.border_width = *p.border_width;
    if (p.speed != 1.0f)            spec.speed = p.speed;
    if (p.seed != 0)                spec.seed = p.seed;
    
    if (p.color_a.has_value()) spec.color_a = AnimatedColorSpec{*p.color_a};
    if (p.color_b.has_value()) spec.color_b = AnimatedColorSpec{*p.color_b};
    if (p.color_c.has_value()) spec.color_c = AnimatedColorSpec{*p.color_c};

    layer.procedural = std::move(spec);
    return layer;
}

} // namespace tachyon::presets
