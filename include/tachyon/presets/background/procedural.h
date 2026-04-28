#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/types/colors.h"

namespace tachyon::presets::background::procedural {

// Common parameter sets for procedural backgrounds

struct ProceduralParams {
    ColorSpec palette_a;
    ColorSpec palette_b;
    ColorSpec palette_c;
    double motion_speed = 1.0;
    double contrast = 1.0;
    double grain = 0.0;
    double softness = 0.0;
    uint64_t seed = 0;
};

// Standard palette presets (using 0-255 range for ColorSpec)
namespace palettes {

inline ProceduralParams dark_tech() {
    return {
        {5, 8, 20, 255},      // Deep navy
        {25, 30, 50, 255},    // Mid blue-grey
        {13, 20, 38, 0},      // Transparent accent
        1.0, 1.0, 0.15, 0.0, 42
    };
}

inline ProceduralParams warm_amber() {
    return {
        {20, 13, 5, 255},     // Dark brown
        {51, 31, 10, 255},    // Warm brown
        {77, 46, 15, 0},      // Glow accent
        0.8, 1.1, 0.10, 0.0, 7
    };
}

inline ProceduralParams cool_mint() {
    return {
        {5, 15, 13, 255},     // Dark teal
        {13, 38, 31, 255},    // Mid teal
        {20, 64, 51, 0},      // Bright accent
        1.2, 0.9, 0.12, 0.0, 123
    };
}

inline ProceduralParams neon_night() {
    return {
        {8, 5, 15, 255},      // Deep purple-black
        {38, 13, 64, 255},    // Purple
        {64, 25, 102, 0},     // Neon accent
        1.5, 1.2, 0.20, 0.0, 99
    };
}

} // namespace palettes

// Background preset factory functions

[[nodiscard]] inline LayerSpec shapegrid(
    int width, int height,
    const ProceduralParams& params = palettes::dark_tech(),
    double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_procedural";
    bg.name = "ShapeGrid Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "grid";
    bg.procedural->seed = params.seed;
    bg.procedural->color_a = AnimatedColorSpec(params.palette_a);
    bg.procedural->color_b = AnimatedColorSpec(params.palette_b);
    bg.procedural->color_c = AnimatedColorSpec(params.palette_c);
    bg.procedural->speed = AnimatedScalarSpec(params.motion_speed);
    bg.procedural->frequency = AnimatedScalarSpec(8.0);
    bg.procedural->amplitude = AnimatedScalarSpec(1.0);
    bg.procedural->scale = AnimatedScalarSpec(1.0);
    bg.procedural->shape = "square";
    bg.procedural->spacing = AnimatedScalarSpec(50.0);
    bg.procedural->border_width = AnimatedScalarSpec(1.0);
    bg.procedural->grain_amount = AnimatedScalarSpec(params.grain);
    bg.procedural->contrast = AnimatedScalarSpec(params.contrast);
    bg.procedural->gamma = AnimatedScalarSpec(1.0);
    bg.procedural->saturation = AnimatedScalarSpec(1.0);
    bg.procedural->softness = AnimatedScalarSpec(params.softness);
    return bg;
}

[[nodiscard]] inline LayerSpec aura(
    int width, int height,
    const ProceduralParams& params = palettes::neon_night(),
    double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_procedural";
    bg.name = "Aura Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "aura";
    bg.procedural->seed = params.seed;
    bg.procedural->color_a = AnimatedColorSpec(params.palette_a);
    bg.procedural->color_b = AnimatedColorSpec(params.palette_b);
    bg.procedural->color_c = AnimatedColorSpec(params.palette_c);
    bg.procedural->speed = AnimatedScalarSpec(params.motion_speed);
    bg.procedural->frequency = AnimatedScalarSpec(3.0);
    bg.procedural->amplitude = AnimatedScalarSpec(1.0);
    bg.procedural->scale = AnimatedScalarSpec(1.0);
    bg.procedural->grain_amount = AnimatedScalarSpec(params.grain);
    bg.procedural->contrast = AnimatedScalarSpec(params.contrast);
    bg.procedural->gamma = AnimatedScalarSpec(1.0);
    bg.procedural->saturation = AnimatedScalarSpec(1.0);
    bg.procedural->softness = AnimatedScalarSpec(params.softness);
    bg.procedural->octave_decay = AnimatedScalarSpec(0.5);
    bg.procedural->band_height = AnimatedScalarSpec(0.5);
    bg.procedural->band_spread = AnimatedScalarSpec(1.0);
    return bg;
}

[[nodiscard]] inline LayerSpec stars(
    int width, int height,
    const ProceduralParams& params = palettes::cool_mint(),
    double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_procedural";
    bg.name = "Stars Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "stars";
    bg.procedural->seed = params.seed;
    bg.procedural->color_a = AnimatedColorSpec(params.palette_a);
    bg.procedural->color_b = AnimatedColorSpec(params.palette_b);
    bg.procedural->color_c = AnimatedColorSpec(params.palette_c);
    bg.procedural->speed = AnimatedScalarSpec(params.motion_speed);
    bg.procedural->frequency = AnimatedScalarSpec(100.0);
    bg.procedural->amplitude = AnimatedScalarSpec(1.0);
    bg.procedural->scale = AnimatedScalarSpec(1.0);
    bg.procedural->grain_amount = AnimatedScalarSpec(params.grain);
    bg.procedural->contrast = AnimatedScalarSpec(params.contrast);
    bg.procedural->gamma = AnimatedScalarSpec(1.0);
    bg.procedural->saturation = AnimatedScalarSpec(1.0);
    bg.procedural->softness = AnimatedScalarSpec(params.softness);
    return bg;
}

[[nodiscard]] inline LayerSpec noise(
    int width, int height,
    const ProceduralParams& params = palettes::dark_tech(),
    double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_procedural";
    bg.name = "Noise Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "noise";
    bg.procedural->seed = params.seed;
    bg.procedural->color_a = AnimatedColorSpec(params.palette_a);
    bg.procedural->color_b = AnimatedColorSpec(params.palette_b);
    bg.procedural->color_c = AnimatedColorSpec(params.palette_c);
    bg.procedural->speed = AnimatedScalarSpec(params.motion_speed);
    bg.procedural->frequency = AnimatedScalarSpec(4.0);
    bg.procedural->amplitude = AnimatedScalarSpec(1.0);
    bg.procedural->scale = AnimatedScalarSpec(1.0);
    bg.procedural->grain_amount = AnimatedScalarSpec(params.grain);
    bg.procedural->contrast = AnimatedScalarSpec(params.contrast);
    bg.procedural->gamma = AnimatedScalarSpec(1.0);
    bg.procedural->saturation = AnimatedScalarSpec(1.0);
    bg.procedural->softness = AnimatedScalarSpec(params.softness);
    return bg;
}

} // namespace tachyon::presets::background::procedural
