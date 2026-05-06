#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/builders_common.h"
#include <optional>
#include <unordered_map>

namespace tachyon::presets {

namespace {

using namespace background::procedural_bg;

ProceduralParams get_palette(const std::string& name) {
    static const std::unordered_map<std::string, ProceduralParams (*)()> palette_map = {
        {"dark_tech", palettes::dark_tech},
        {"warm_amber", palettes::warm_amber},
        {"cool_mint", palettes::cool_mint},
        {"neon_night", palettes::neon_night},
        {"pure_white", palettes::pure_white},
        {"clean_white", palettes::clean_white},
        {"warm_white", palettes::warm_white},
        {"soft_white", palettes::soft_white},
        {"premium_dark", palettes::premium_dark},
        {"neon_grid", palettes::neon_grid},
        {"midnight_silk", palettes::midnight_silk},
        {"golden_horizon", palettes::golden_horizon},
        {"cyber_matrix", palettes::cyber_matrix},
        {"frosted_glass", palettes::frosted_glass},
        {"cosmic_nebula", palettes::cosmic_nebula},
        {"brushed_metal", palettes::brushed_metal},
        {"oceanic_abyss", palettes::oceanic_abyss},
        {"royal_velvet", palettes::royal_velvet},
        {"prismatic_light", palettes::prismatic_light}
    };

    auto it = palette_map.find(name);
    if (it != palette_map.end()) {
        return it->second();
    }
    return palettes::premium_dark();
}

void apply_background_overrides(ProceduralSpec& spec, const BackgroundParams& p) {
    if (p.frequency)    spec.frequency = *p.frequency;
    if (p.amplitude)    spec.amplitude = *p.amplitude;
    if (p.scale)        spec.scale = *p.scale;
    if (p.contrast)     spec.contrast = *p.contrast;
    if (p.softness)     spec.softness = *p.softness;
    if (p.grain_amount) spec.grain_amount = *p.grain_amount;
    if (p.shape)        spec.shape = *p.shape;
    if (p.spacing)      spec.spacing = *p.spacing;
    if (p.border_width) spec.border_width = *p.border_width;
    if (p.speed != 1.0f) spec.speed = p.speed;
    if (p.seed != 0)     spec.seed = p.seed;

    if (p.color_a) spec.color_a = AnimatedColorSpec{*p.color_a};
    if (p.color_b) spec.color_b = AnimatedColorSpec{*p.color_b};
    if (p.color_c) spec.color_c = AnimatedColorSpec{*p.color_c};
}

ProceduralSpec make_unresolved_background_spec(std::string_view kind) {
    ProceduralSpec spec;
    spec.kind = std::string(kind);
    return spec;
}

} // namespace

LayerSpec build_background(const BackgroundParams& p) {
    auto palette = get_palette(p.palette);
    if (p.seed != 0) palette.seed = p.seed;
    palette.motion_speed *= static_cast<double>(p.speed);
    
    // LayerParams order: in_point, out_point, x, y, w, h, opacity
    LayerSpec layer = make_base_layer("bg_layer", "Background", "procedural", {
        p.in_point, p.out_point, p.x, p.y, p.w, p.h, p.opacity
    });

    const auto& registry = BackgroundKindRegistry::instance();

    std::optional<ProceduralSpec> spec;
    if (!p.kind.empty()) {
        registry::ParameterBag bag;
        bag.set("palette.a", palette.palette_a);
        bag.set("palette.b", palette.palette_b);
        bag.set("palette.c", palette.palette_c);
        bag.set("motion_speed", palette.motion_speed);
        bag.set("contrast", palette.contrast);
        bag.set("grain", palette.grain);
        bag.set("softness", palette.softness);
        bag.set("seed", static_cast<int>(palette.seed));
        
        if (p.spacing) bag.set("spacing", *p.spacing);
        if (p.border_width) bag.set("border_width", *p.border_width);
        if (p.mouse_influence) bag.set("mouse_influence", *p.mouse_influence);
        if (p.mouse_radius) bag.set("mouse_radius", *p.mouse_radius);
        bag.set("mouse_x", p.mouse_x);
        bag.set("mouse_y", p.mouse_y);
        if (p.shape) bag.set("shape", *p.shape);

        spec = registry.create(p.kind, bag);
        if (!spec) {
            // Preserve the requested kind even when no procedural factory exists.
            spec = make_unresolved_background_spec(p.kind);
        }
    }

    if (spec) {
        apply_background_overrides(*spec, p);
        layer.procedural = std::move(spec);
        layer.type = LayerType::Procedural;
        layer.type_string.clear();
    } else {
        layer.type = LayerType::NullLayer;
        layer.type_string.clear();
        layer.enabled = false;
    }

    layer.preset_id = p.kind;
    return layer;
}

} // namespace tachyon::presets
