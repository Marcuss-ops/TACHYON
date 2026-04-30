#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/types/colors.h"


namespace tachyon::presets::background::procedural_bg {

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
  return {{5, 8, 20, 255},   // Deep navy
          {25, 30, 50, 255}, // Mid blue-grey
          {13, 20, 38, 0},   // Transparent accent
          1.0,
          1.0,
          0.15,
          0.0,
          42};
}

inline ProceduralParams warm_amber() {
  return {{20, 13, 5, 255},  // Dark brown
          {51, 31, 10, 255}, // Warm brown
          {77, 46, 15, 0},   // Glow accent
          0.8,
          1.1,
          0.10,
          0.0,
          7};
}

inline ProceduralParams cool_mint() {
  return {{5, 15, 13, 255},  // Dark teal
          {13, 38, 31, 255}, // Mid teal
          {20, 64, 51, 0},   // Bright accent
          1.2,
          0.9,
          0.12,
          0.0,
          123};
}

inline ProceduralParams neon_night() {
  return {{8, 5, 15, 255},   // Deep purple-black
          {38, 13, 64, 255}, // Purple
          {64, 25, 102, 0},  // Neon accent
          1.5,
          1.2,
          0.20,
          0.0,
          99};
}

// Modern Minimalist White Palettes
inline ProceduralParams pure_white() {
  return {{255, 255, 255, 255}, // Pure white
          {248, 248, 248, 255}, // Slightly off-white
          {240, 240, 240, 0},   // Light grey accent
          0.5,
          1.0,
          0.05,
          0.3,
          1001};
}

inline ProceduralParams clean_white() {
  return {{252, 252, 252, 255}, // Clean white
          {245, 245, 247, 255}, // Cool white
          {235, 235, 240, 0},   // Subtle accent
          0.3,
          1.0,
          0.03,
          0.2,
          2002};
}

inline ProceduralParams warm_white() {
  return {{255, 253, 250, 255}, // Warm white (slight yellow tint)
          {250, 248, 245, 255}, // Warmer off-white
          {245, 242, 238, 0},   // Warm accent
          0.4,
          1.0,
          0.04,
          0.25,
          3003};
}

inline ProceduralParams soft_white() {
  return {{250, 250, 252, 255}, // Soft white with blue tint
          {242, 244, 247, 255}, // Gentle grey-white
          {235, 238, 242, 0},   // Soft accent
          0.6,
          0.95,
          0.02,
          0.15,
          4004};
}

inline ProceduralParams premium_dark() {
  return {{10, 10, 10, 255}, // Deep black
          {26, 26, 46, 255}, // Midnight blue
          {15, 15, 25, 255}, // Dark accent
          0.5,
          1.05,
          0.08,
          0.0,
          5005};
}

inline ProceduralParams neon_grid() {
  return {{5, 5, 5, 255},     // Black
          {0, 242, 255, 255}, // Neon cyan
          {0, 0, 0, 0},       1.0, 1.1, 0.05, 0.0, 6006};
}

inline ProceduralParams midnight_silk() {
  return {{10, 12, 18, 255}, // Midnight navy
          {25, 28, 35, 255}, // Charcoal silk
          {15, 15, 20, 255}, // Deep accent
          0.4,
          1.05,
          0.05,
          0.2,
          777};
}

inline ProceduralParams golden_horizon() {
  return {{45, 30, 10, 255},  // Deep amber
          {85, 60, 20, 255},  // Golden glow
          {120, 90, 40, 255}, // Highlight gold
          0.3,
          1.1,
          0.12,
          0.5,
          888};
}

inline ProceduralParams cyber_matrix() {
  return {{10, 5, 20, 255},    // Dark matrix void
          {180, 50, 255, 255}, // Neon violet
          {50, 200, 255, 255}, // Neon cyan
          1.2,
          1.2,
          0.05,
          0.0,
          999};
}

inline ProceduralParams frosted_glass() {
  return {{240, 245, 255, 255}, // Ice blue
          {220, 230, 250, 255}, // Soft shadow
          {255, 255, 255, 255}, // Pure highlight
          0.2,
          0.9,
          0.03,
          0.8,
          111};
}

inline ProceduralParams cosmic_nebula() {
  return {{5, 5, 15, 255},    // Deep space
          {80, 20, 120, 255}, // Nebula purple
          {20, 60, 100, 255}, // Nebula blue
          0.8,
          1.15,
          0.1,
          0.3,
          222};
}

inline ProceduralParams brushed_metal() {
  return {{40, 40, 45, 255},    // Dark steel
          {70, 70, 75, 255},    // Mid steel
          {100, 100, 110, 255}, // Light steel highlight
          0.1,
          1.1,
          0.25,
          0.1,
          1234};
}

inline ProceduralParams oceanic_abyss() {
  return {{2, 15, 30, 255},   // Deep ocean floor
          {5, 45, 60, 255},   // Mid-water teal
          {10, 80, 100, 255}, // Light rays highlight
          0.5,
          1.0,
          0.05,
          0.7,
          5678};
}

inline ProceduralParams royal_velvet() {
  return {{40, 0, 10, 255},  // Deep crimson shadow
          {80, 5, 30, 255},  // Royal velvet base
          {60, 10, 50, 255}, // Purple highlight
          0.3,
          1.2,
          0.08,
          0.4,
          9012};
}

inline ProceduralParams prismatic_light() {
  return {{255, 240, 245, 255}, // Soft pink highlight
          {240, 255, 250, 255}, // Soft mint highlight
          {245, 245, 255, 255}, // Soft blue highlight
          0.4,
          0.95,
          0.02,
          0.6,
          3456};
}

inline ProceduralParams technical_blueprint() {
  return {{10, 30, 70, 255},    // Blueprint blue
          {255, 255, 255, 255}, // Pure white lines
          {15, 45, 90, 255},    // Darker blue accent
          0.0,
          1.0,
          0.0,
          0.0,
          7890};
}

} // namespace palettes

// Background preset factory functions

[[nodiscard]] inline LayerSpec
modern_tech_grid(int width, int height,
                 const ProceduralParams &params = palettes::neon_grid(),
                 double spacing = 60.0, double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_grid_modern";
  bg.name = "Modern Tech Grid";
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
  bg.procedural->kind = "grid_lines";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->spacing = AnimatedScalarSpec{spacing};
  bg.procedural->border_width = AnimatedScalarSpec{1.2};
  bg.procedural->scanline_intensity = AnimatedScalarSpec{0.15};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  return bg;
}

[[nodiscard]] inline LayerSpec
classico_premium(int width, int height,
                 const ProceduralParams &params = palettes::premium_dark(),
                 double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_classico_premium";
  bg.name = "Classico Premium";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->frequency = AnimatedScalarSpec{0.8};
  bg.procedural->warp_strength = AnimatedScalarSpec{2.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->saturation = AnimatedScalarSpec{0.9};
  bg.procedural->contrast = AnimatedScalarSpec{1.05};
  return bg;
}

[[nodiscard]] inline LayerSpec
shapegrid(int width, int height,
          const ProceduralParams &params = palettes::dark_tech(),
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{8.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->shape = "square";
  bg.procedural->spacing = AnimatedScalarSpec{50.0};
  bg.procedural->border_width = AnimatedScalarSpec{1.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{1.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
aura(int width, int height,
     const ProceduralParams &params = palettes::neon_night(),
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{3.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{1.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  bg.procedural->octave_decay = AnimatedScalarSpec{0.5};
  bg.procedural->band_height = AnimatedScalarSpec{0.5};
  bg.procedural->band_spread = AnimatedScalarSpec{1.0};
  return bg;
}

[[nodiscard]] inline LayerSpec
stars(int width, int height,
      const ProceduralParams &params = palettes::cool_mint(),
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{100.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{1.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
noise(int width, int height,
      const ProceduralParams &params = palettes::dark_tech(),
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{4.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{1.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

// Modern Minimalist White Backgrounds

[[nodiscard]] inline LayerSpec
minimal_white(int width, int height,
              const ProceduralParams &params = palettes::pure_white(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_minimal_white";
  bg.name = "Minimal White Background";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->frequency = AnimatedScalarSpec{0.0};
  bg.procedural->amplitude = AnimatedScalarSpec{0.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{0.02};
  bg.procedural->contrast = AnimatedScalarSpec{1.0};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{0.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
clean_grid(int width, int height,
           const ProceduralParams &params = palettes::clean_white(),
           double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_clean_grid";
  bg.name = "Clean Grid Background";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{{220, 220, 225, 255}};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{12.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->shape = "square";
  bg.procedural->spacing = AnimatedScalarSpec{80.0};
  bg.procedural->border_width = AnimatedScalarSpec{0.5};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
subtle_texture(int width, int height,
               const ProceduralParams &params = palettes::soft_white(),
               double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_subtle_texture";
  bg.name = "Subtle Texture Background";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{16.0};
  bg.procedural->amplitude = AnimatedScalarSpec{0.3};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{0.08};
  bg.procedural->contrast = AnimatedScalarSpec{1.0};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{0.1};
  bg.procedural->softness = AnimatedScalarSpec{0.4};
  return bg;
}

[[nodiscard]] inline LayerSpec
soft_gradient(int width, int height,
              const ProceduralParams &params = palettes::warm_white(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_soft_gradient";
  bg.name = "Soft Gradient Background";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{{248, 248, 250, 0}};
  bg.procedural->speed = AnimatedScalarSpec{0.2};
  bg.procedural->frequency = AnimatedScalarSpec{2.0};
  bg.procedural->amplitude = AnimatedScalarSpec{0.5};
  bg.procedural->scale = AnimatedScalarSpec{1.5};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{0.95};
  bg.procedural->gamma = AnimatedScalarSpec{1.0};
  bg.procedural->saturation = AnimatedScalarSpec{0.05};
  bg.procedural->softness = AnimatedScalarSpec{0.8};
  bg.procedural->octave_decay = AnimatedScalarSpec{0.5};
  bg.procedural->band_height = AnimatedScalarSpec{0.3};
  return bg;
}

[[nodiscard]] inline LayerSpec
elegant_dots(int width, int height,
             const ProceduralParams &params = palettes::clean_white(),
             double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_elegant_dots";
  bg.name = "Elegant Dots Background";
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
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{{230, 230, 235, 255}};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{0.1};
  bg.procedural->frequency = AnimatedScalarSpec{20.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.0};
  bg.procedural->scale = AnimatedScalarSpec{1.0};
  bg.procedural->shape = "circle";
  bg.procedural->spacing = AnimatedScalarSpec{60.0};
  bg.procedural->border_width = AnimatedScalarSpec{0.3};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{0.3};
  return bg;
}

// Premium Style Backgrounds

[[nodiscard]] inline LayerSpec
midnight_silk(int width, int height,
              const ProceduralParams &params = palettes::midnight_silk(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_midnight_silk";
  bg.name = "Midnight Silk";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "aura";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{1.5};
  bg.procedural->warp_strength = AnimatedScalarSpec{4.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
golden_horizon(int width, int height,
               const ProceduralParams &params = palettes::golden_horizon(),
               double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_golden_horizon";
  bg.name = "Golden Horizon";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "aura";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{2.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  bg.procedural->saturation = AnimatedScalarSpec{1.2};
  return bg;
}

[[nodiscard]] inline LayerSpec
cyber_matrix(int width, int height,
             const ProceduralParams &params = palettes::cyber_matrix(),
             double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_cyber_matrix";
  bg.name = "Cyber Matrix";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "grid_lines";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->spacing = AnimatedScalarSpec{40.0};
  bg.procedural->border_width = AnimatedScalarSpec{0.8};
  bg.procedural->scanline_intensity = AnimatedScalarSpec{0.3};
  bg.procedural->warp_strength = AnimatedScalarSpec{0.5};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  return bg;
}

[[nodiscard]] inline LayerSpec
frosted_glass(int width, int height,
              const ProceduralParams &params = palettes::frosted_glass(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_frosted_glass";
  bg.name = "Frosted Glass";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "noise";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->frequency = AnimatedScalarSpec{8.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{0.1};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->gamma = AnimatedScalarSpec{1.1};
  return bg;
}

[[nodiscard]] inline LayerSpec
cosmic_nebula(int width, int height,
              const ProceduralParams &params = palettes::cosmic_nebula(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_cosmic_nebula";
  bg.name = "Cosmic Nebula";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "aura";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{4.0};
  bg.procedural->warp_strength = AnimatedScalarSpec{6.0};
  bg.procedural->warp_frequency = AnimatedScalarSpec{2.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->saturation = AnimatedScalarSpec{1.1};
  return bg;
}

[[nodiscard]] inline LayerSpec
brushed_metal(int width, int height,
              const ProceduralParams &params = palettes::brushed_metal(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_brushed_metal";
  bg.name = "Brushed Metal";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "noise";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->frequency = AnimatedScalarSpec{12.0};
  bg.procedural->amplitude = AnimatedScalarSpec{0.8};
  bg.procedural->scale = AnimatedScalarSpec{2.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
oceanic_abyss(int width, int height,
              const ProceduralParams &params = palettes::oceanic_abyss(),
              double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_oceanic_abyss";
  bg.name = "Oceanic Abyss";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "waves";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{3.0};
  bg.procedural->amplitude = AnimatedScalarSpec{1.5};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  return bg;
}

[[nodiscard]] inline LayerSpec
royal_velvet(int width, int height,
             const ProceduralParams &params = palettes::royal_velvet(),
             double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_royal_velvet";
  bg.name = "Royal Velvet";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "aura";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{1.2};
  bg.procedural->warp_strength = AnimatedScalarSpec{5.0};
  bg.procedural->grain_amount = AnimatedScalarSpec{params.grain};
  bg.procedural->contrast = AnimatedScalarSpec{params.contrast};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  return bg;
}

[[nodiscard]] inline LayerSpec
prismatic_light(int width, int height,
                const ProceduralParams &params = palettes::prismatic_light(),
                double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_prismatic_light";
  bg.name = "Prismatic Light";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "aura";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->speed = AnimatedScalarSpec{params.motion_speed};
  bg.procedural->frequency = AnimatedScalarSpec{3.0};
  bg.procedural->warp_strength = AnimatedScalarSpec{8.0};
  bg.procedural->softness = AnimatedScalarSpec{params.softness};
  bg.procedural->contrast = AnimatedScalarSpec{0.9};
  bg.procedural->saturation = AnimatedScalarSpec{0.5};
  return bg;
}

[[nodiscard]] inline LayerSpec technical_blueprint(
    int width, int height,
    const ProceduralParams &params = palettes::technical_blueprint(),
    double duration = 5.0) {
  LayerSpec bg;
  bg.id = "bg_technical_blueprint";
  bg.name = "Technical Blueprint";
  bg.type = "procedural";
  bg.width = width;
  bg.height = height;
  bg.out_point = duration;
  bg.procedural = ProceduralSpec{};
  bg.procedural->kind = "grid_lines";
  bg.procedural->seed = params.seed;
  bg.procedural->color_a = AnimatedColorSpec{params.palette_a};
  bg.procedural->color_b = AnimatedColorSpec{params.palette_b};
  bg.procedural->color_c = AnimatedColorSpec{params.palette_c};
  bg.procedural->spacing = AnimatedScalarSpec{100.0};
  bg.procedural->border_width = AnimatedScalarSpec{0.5};
  bg.procedural->scanline_intensity = AnimatedScalarSpec{0.1};
  bg.procedural->contrast = AnimatedScalarSpec{1.1};
  return bg;
}

} // namespace tachyon::presets::background::procedural_bg
