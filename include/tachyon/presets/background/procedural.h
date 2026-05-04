#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/types/colors.h"

namespace tachyon::presets::background {

struct ShapeGridParams {
    std::string shape = "square";
    float spacing = 40.0f;
    float border_width = 1.0f;
    ColorSpec border_color = {150, 150, 150, 255};
    ColorSpec background_color = {0, 0, 0, 255};
    ColorSpec grid_color = {150, 150, 150, 255};
    float speed = 1.0f;
    std::string direction = "right";
    uint64_t seed = 0;
};

inline ProceduralSpec generate_shape_grid_spec(const ShapeGridParams& params) {
    ProceduralSpec spec;
    spec.kind = "grid";
    spec.shape = params.shape;
    spec.spacing = params.spacing;
    spec.border_width = params.border_width;
    spec.border_color = params.border_color;
    spec.color_a = params.background_color;
    spec.color_b = params.grid_color;
    spec.speed = params.speed;
    spec.seed = params.seed;
    if (params.direction == "right")       spec.angle = 0.0;
    else if (params.direction == "left")  spec.angle = 180.0;
    else if (params.direction == "up")    spec.angle = 90.0;
    else if (params.direction == "down")  spec.angle = 270.0;
    else if (params.direction == "diagonal") spec.angle = 45.0;
    else spec.angle = 0.0;
    return spec;
}

} // namespace tachyon::presets::background

namespace tachyon::presets::background::procedural_bg {

struct ProceduralParams {
  ColorSpec palette_a;
  ColorSpec palette_b;
  ColorSpec palette_c;
  double motion_speed = 1.0;
  double contrast = 1.0;
  double grain = 0.0;
  double softness = 0.0;
  uint64_t seed = 0;

  ProceduralParams() = default;
  ProceduralParams(ColorSpec a, ColorSpec b, ColorSpec c, double speed, double contrast, double grain, double softness, uint64_t seed)
    : palette_a(a), palette_b(b), palette_c(c), motion_speed(speed), contrast(contrast), grain(grain), softness(softness), seed(seed) {}
};

namespace palettes {

inline ProceduralParams dark_tech() {
  return {{5, 8, 20, 255}, {25, 30, 50, 255}, {13, 20, 38, 0}, 1.0, 1.0, 0.15, 0.0, 42};
}

inline ProceduralParams warm_amber() {
  return {{20, 13, 5, 255}, {51, 31, 10, 255}, {77, 46, 15, 0}, 0.8, 1.1, 0.10, 0.0, 7};
}

inline ProceduralParams cool_mint() {
  return {{5, 15, 13, 255}, {13, 38, 31, 255}, {20, 64, 51, 0}, 1.2, 0.9, 0.12, 0.0, 123};
}

inline ProceduralParams neon_night() {
  return {{8, 5, 15, 255}, {38, 13, 64, 255}, {64, 25, 102, 0}, 1.5, 1.2, 0.20, 0.0, 99};
}

inline ProceduralParams pure_white() {
  return {{255, 255, 255, 255}, {248, 248, 248, 255}, {240, 240, 240, 0}, 0.5, 1.0, 0.05, 0.3, 1001};
}

inline ProceduralParams clean_white() {
  return {{252, 252, 252, 255}, {245, 245, 247, 255}, {235, 235, 240, 0}, 0.3, 1.0, 0.03, 0.2, 2002};
}

inline ProceduralParams warm_white() {
  return {{255, 253, 250, 255}, {250, 248, 245, 255}, {245, 242, 238, 0}, 0.4, 1.0, 0.04, 0.25, 3003};
}

inline ProceduralParams soft_white() {
  return {{250, 250, 252, 255}, {242, 244, 247, 255}, {235, 238, 242, 0}, 0.6, 0.95, 0.02, 0.15, 4004};
}

inline ProceduralParams premium_dark() {
  return {{10, 10, 10, 255}, {26, 26, 46, 255}, {15, 15, 25, 255}, 0.5, 1.05, 0.08, 0.0, 5005};
}

inline ProceduralParams neon_grid() {
  return {{5, 5, 5, 255}, {0, 242, 255, 255}, {0, 0, 0, 0}, 1.0, 1.1, 0.05, 0.0, 6006};
}

inline ProceduralParams neon_ripple() {
  return {{10, 5, 25, 255}, {106, 59, 255, 255}, {0, 242, 255, 255}, 1.2, 1.2, 0.08, 0.0, 7007};
}

inline ProceduralParams midnight_silk() {
  return {{10, 12, 18, 255}, {25, 28, 35, 255}, {15, 15, 20, 255}, 0.4, 1.05, 0.05, 0.2, 777};
}

inline ProceduralParams golden_horizon() {
  return {{45, 30, 10, 255}, {85, 60, 20, 255}, {120, 90, 40, 255}, 0.3, 1.1, 0.12, 0.5, 888};
}

inline ProceduralParams cyber_matrix() {
  return {{10, 5, 20, 255}, {180, 50, 255, 255}, {50, 200, 255, 255}, 1.2, 1.2, 0.05, 0.0, 999};
}

inline ProceduralParams frosted_glass() {
  return {{240, 245, 255, 255}, {220, 230, 250, 255}, {255, 255, 255, 255}, 0.2, 0.9, 0.03, 0.8, 111};
}

inline ProceduralParams cosmic_nebula() {
  return {{5, 5, 15, 255}, {80, 20, 120, 255}, {20, 60, 100, 255}, 0.8, 1.15, 0.1, 0.3, 222};
}

inline ProceduralParams brushed_metal() {
  return {{40, 40, 45, 255}, {70, 70, 75, 255}, {100, 100, 110, 255}, 0.1, 1.1, 0.25, 0.1, 1234};
}

inline ProceduralParams oceanic_abyss() {
  return {{2, 15, 30, 255}, {5, 45, 60, 255}, {10, 80, 100, 255}, 0.5, 1.0, 0.05, 0.7, 5678};
}

inline ProceduralParams royal_velvet() {
  return {{40, 0, 10, 255}, {80, 5, 30, 255}, {60, 10, 50, 255}, 0.3, 1.2, 0.08, 0.4, 9012};
}

inline ProceduralParams prismatic_light() {
  return {{255, 240, 245, 255}, {240, 255, 250, 255}, {245, 245, 255, 255}, 0.4, 0.95, 0.02, 0.6, 3456};
}

} // namespace palettes

// Procedural Spec factories

inline ProceduralSpec make_modern_tech_grid_spec(const ProceduralParams &params = palettes::neon_grid(), double spacing = 60.0) {
  ProceduralSpec spec;
  spec.kind = "grid_lines";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.spacing = AnimatedScalarSpec{spacing};
  spec.border_width = AnimatedScalarSpec{1.2};
  spec.scanline_intensity = AnimatedScalarSpec{0.15};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  return spec;
}

inline ProceduralSpec make_aura_spec(const ProceduralParams &params = palettes::neon_night()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{3.0};
  spec.amplitude = AnimatedScalarSpec{1.0};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.gamma = AnimatedScalarSpec{1.0};
  spec.saturation = AnimatedScalarSpec{1.0};
  spec.softness = AnimatedScalarSpec{params.softness};
  spec.octave_decay = AnimatedScalarSpec{0.5};
  spec.band_height = AnimatedScalarSpec{0.5};
  spec.band_spread = AnimatedScalarSpec{1.0};
  return spec;
}

inline ProceduralSpec make_stars_spec(const ProceduralParams &params = palettes::cool_mint()) {
  ProceduralSpec spec;
  spec.kind = "stars";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{100.0};
  spec.amplitude = AnimatedScalarSpec{1.0};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.gamma = AnimatedScalarSpec{1.0};
  spec.saturation = AnimatedScalarSpec{1.0};
  spec.softness = AnimatedScalarSpec{params.softness};
  return spec;
}

inline ProceduralSpec make_noise_spec(const ProceduralParams &params = palettes::dark_tech()) {
  ProceduralSpec spec;
  spec.kind = "noise";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{4.0};
  spec.amplitude = AnimatedScalarSpec{1.0};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.gamma = AnimatedScalarSpec{1.0};
  spec.saturation = AnimatedScalarSpec{1.0};
  spec.softness = AnimatedScalarSpec{params.softness};
  return spec;
}

inline ProceduralSpec make_classico_premium_spec(const ProceduralParams &params = palettes::premium_dark()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.frequency = AnimatedScalarSpec{0.8};
  spec.warp_strength = AnimatedScalarSpec{2.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.saturation = AnimatedScalarSpec{0.9};
  spec.contrast = AnimatedScalarSpec{1.05};
  return spec;
}

inline ProceduralSpec make_clean_grid_spec(const ProceduralParams &params = palettes::clean_white()) {
  ProceduralSpec spec;
  spec.kind = "grid";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{{220, 220, 225, 255}};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{12.0};
  spec.amplitude = AnimatedScalarSpec{1.0};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.shape = "square";
  spec.spacing = AnimatedScalarSpec{80.0};
  spec.border_width = AnimatedScalarSpec{0.5};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.softness = AnimatedScalarSpec{params.softness};
  return spec;
}

inline ProceduralSpec make_subtle_texture_spec(const ProceduralParams &params = palettes::soft_white()) {
  ProceduralSpec spec;
  spec.kind = "noise";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{16.0};
  spec.amplitude = AnimatedScalarSpec{0.3};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.grain_amount = AnimatedScalarSpec{0.08};
  spec.contrast = AnimatedScalarSpec{1.0};
  spec.gamma = AnimatedScalarSpec{1.0};
  spec.saturation = AnimatedScalarSpec{0.1};
  spec.softness = AnimatedScalarSpec{0.4};
  return spec;
}

inline ProceduralSpec make_soft_gradient_spec(const ProceduralParams &params = palettes::warm_white()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{{248, 248, 250, 0}};
  spec.speed = AnimatedScalarSpec{0.2};
  spec.frequency = AnimatedScalarSpec{2.0};
  spec.amplitude = AnimatedScalarSpec{0.5};
  spec.scale = AnimatedScalarSpec{1.5};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{0.95};
  spec.gamma = AnimatedScalarSpec{1.0};
  spec.saturation = AnimatedScalarSpec{0.05};
  spec.softness = AnimatedScalarSpec{0.8};
  spec.octave_decay = AnimatedScalarSpec{0.5};
  spec.band_height = AnimatedScalarSpec{0.3};
  return spec;
}

inline ProceduralSpec make_elegant_dots_spec(const ProceduralParams &params = palettes::clean_white()) {
  ProceduralSpec spec;
  spec.kind = "grid";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{{230, 230, 235, 255}};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{0.1};
  spec.frequency = AnimatedScalarSpec{20.0};
  spec.amplitude = AnimatedScalarSpec{1.0};
  spec.scale = AnimatedScalarSpec{1.0};
  spec.shape = "circle";
  spec.spacing = AnimatedScalarSpec{60.0};
  spec.border_width = AnimatedScalarSpec{0.3};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.softness = AnimatedScalarSpec{0.3};
  return spec;
}

inline ProceduralSpec make_midnight_silk_spec(const ProceduralParams &params = palettes::midnight_silk()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{1.5};
  spec.warp_strength = AnimatedScalarSpec{4.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.softness = AnimatedScalarSpec{params.softness};
  return spec;
}

inline ProceduralSpec make_golden_horizon_spec(const ProceduralParams &params = palettes::golden_horizon()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{2.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.softness = AnimatedScalarSpec{params.softness};
  spec.saturation = AnimatedScalarSpec{1.2};
  return spec;
}

inline ProceduralSpec make_cyber_matrix_spec(const ProceduralParams &params = palettes::cyber_matrix()) {
  ProceduralSpec spec;
  spec.kind = "grid_lines";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.spacing = AnimatedScalarSpec{40.0};
  spec.border_width = AnimatedScalarSpec{0.8};
  spec.scanline_intensity = AnimatedScalarSpec{0.3};
  spec.warp_strength = AnimatedScalarSpec{0.5};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  return spec;
}

inline ProceduralSpec make_frosted_glass_spec(const ProceduralParams &params = palettes::frosted_glass()) {
  ProceduralSpec spec;
  spec.kind = "noise";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.frequency = AnimatedScalarSpec{8.0};
  spec.grain_amount = AnimatedScalarSpec{0.1};
  spec.softness = AnimatedScalarSpec{params.softness};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.gamma = AnimatedScalarSpec{1.1};
  return spec;
}

inline ProceduralSpec make_cosmic_nebula_spec(const ProceduralParams &params = palettes::cosmic_nebula()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.frequency = AnimatedScalarSpec{4.0};
  spec.warp_strength = AnimatedScalarSpec{6.0};
  spec.warp_frequency = AnimatedScalarSpec{2.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{params.contrast};
  spec.saturation = AnimatedScalarSpec{1.1};
  return spec;
}

inline ProceduralSpec make_brushed_metal_spec(const ProceduralParams &params = palettes::brushed_metal()) {
  ProceduralSpec spec;
  spec.kind = "noise";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.frequency = AnimatedScalarSpec{32.0};
  spec.warp_strength = AnimatedScalarSpec{0.5};
  spec.grain_amount = AnimatedScalarSpec{0.15};
  spec.contrast = AnimatedScalarSpec{1.1};
  return spec;
}

inline ProceduralSpec make_oceanic_abyss_spec(const ProceduralParams &params = palettes::oceanic_abyss()) {
  ProceduralSpec spec;
  spec.kind = "aura";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{0.3};
  spec.frequency = AnimatedScalarSpec{2.5};
  spec.warp_strength = AnimatedScalarSpec{5.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  spec.contrast = AnimatedScalarSpec{1.0};
  spec.softness = AnimatedScalarSpec{0.7};
  return spec;
}

inline ProceduralSpec make_ripple_grid_spec(const ProceduralParams &params = palettes::neon_ripple()) {
  ProceduralSpec spec;
  spec.kind = "grid_ripple";
  spec.seed = params.seed;
  spec.color_a = AnimatedColorSpec{params.palette_a};
  spec.color_b = AnimatedColorSpec{params.palette_b};
  spec.color_c = AnimatedColorSpec{params.palette_c};
  spec.speed = AnimatedScalarSpec{params.motion_speed};
  spec.spacing = AnimatedScalarSpec{50.0};
  spec.warp_strength = AnimatedScalarSpec{2.0};
  spec.grain_amount = AnimatedScalarSpec{params.grain};
  return spec;
}

} // namespace tachyon::presets::background::procedural_bg
