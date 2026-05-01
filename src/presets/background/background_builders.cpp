#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"

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

    if (p.kind == "grid_modern") return modern_tech_grid(w, h, palette, 60.0, dur);
    if (p.kind == "grid")        return clean_grid(w, h, palette, dur);
    if (p.kind == "aura")        return aura(w, h, palette, dur);
    if (p.kind == "stars")       return stars(w, h, palette, dur);
    if (p.kind == "noise")       return noise(w, h, palette, dur);
    if (p.kind == "minimal_white") return minimal_white(w, h, palette, dur);
    if (p.kind == "texture")     return subtle_texture(w, h, palette, dur);
    if (p.kind == "soft_gradient") return soft_gradient(w, h, palette, dur);
    if (p.kind == "dots")        return elegant_dots(w, h, palette, dur);
    if (p.kind == "silk")        return midnight_silk(w, h, palette, dur);
    if (p.kind == "horizon")     return golden_horizon(w, h, palette, dur);
    if (p.kind == "cyber")       return cyber_matrix(w, h, palette, dur);
    if (p.kind == "glass")       return frosted_glass(w, h, palette, dur);
    if (p.kind == "nebula")      return cosmic_nebula(w, h, palette, dur);
    if (p.kind == "metal")       return brushed_metal(w, h, palette, dur);
    if (p.kind == "ocean")       return oceanic_abyss(w, h, palette, dur);
    
    return classico_premium(w, h, palette, dur);
}

} // namespace tachyon::presets
