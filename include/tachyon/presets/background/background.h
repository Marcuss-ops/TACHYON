#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/types/colors.h"
#include <string>

namespace tachyon::presets::background {

/**
 * @brief Options for configuring background layers.
 */
struct BackgroundOptions {
    std::string id = "bg";
    int width = 1920;
    int height = 1080;
    double duration = 5.0;
    uint64_t seed = 42;
    ColorSpec clear_color = {13, 17, 23, 255};
};

/**
 * @brief Options for procedural aura backgrounds.
 */
struct AuraOptions : BackgroundOptions {
    ColorSpec palette_a = {8, 5, 15, 255};
    ColorSpec palette_b = {38, 13, 64, 255};
    ColorSpec palette_c = {64, 25, 102, 0};
    double speed = 1.5;
    double frequency = 3.0;
    double amplitude = 1.0;
    double scale = 1.0;
    double grain = 0.20;
    double contrast = 1.2;
    double saturation = 1.0;
    double softness = 0.0;
    double octave_decay = 0.5;
    double band_height = 0.5;
    double band_spread = 1.0;
};

/**
 * @brief Options for shape grid backgrounds.
 */
struct ShapeGridOptions : BackgroundOptions {
    ColorSpec palette_a = {5, 8, 20, 255};
    ColorSpec palette_b = {25, 30, 50, 255};
    ColorSpec palette_c = {13, 20, 38, 0};
    double speed = 1.0;
    double frequency = 8.0;
    double amplitude = 1.0;
    double scale = 1.0;
    std::string shape = "square";
    double spacing = 50.0;
    double border_width = 1.0;
    double grain = 0.15;
    double contrast = 1.0;
    double saturation = 1.0;
    double softness = 0.0;
};

/**
 * @brief Options for stars backgrounds.
 */
struct StarsOptions : BackgroundOptions {
    ColorSpec palette_a = {5, 15, 13, 255};
    ColorSpec palette_b = {13, 38, 31, 255};
    ColorSpec palette_c = {20, 64, 51, 0};
    double speed = 1.2;
    double frequency = 100.0;
    double amplitude = 1.0;
    double scale = 1.0;
    double grain = 0.12;
    double contrast = 0.9;
    double saturation = 1.0;
    double softness = 0.0;
};

/**
 * @brief Options for noise backgrounds.
 */
struct NoiseOptions : BackgroundOptions {
    ColorSpec palette_a = {5, 8, 20, 255};
    ColorSpec palette_b = {25, 30, 50, 255};
    ColorSpec palette_c = {13, 20, 38, 0};
    double speed = 1.0;
    double frequency = 4.0;
    double amplitude = 1.0;
    double scale = 1.0;
    double grain = 0.15;
    double contrast = 1.0;
    double saturation = 1.0;
    double softness = 0.0;
};

/**
 * @brief Create an aura procedural background layer.
 * 
 * @code
 * c.layer(background::aura({
 *     .id = "bg",
 *     .width = 1920,
 *     .height = 1080,
 *     .duration = 8.0,
 *     .seed = 42
 * }));
 * @endcode
 */
[[nodiscard]] inline LayerSpec aura(const AuraOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "Aura Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "aura";
    bg.procedural->seed = opts.seed;
    bg.procedural->color_a = AnimatedColorSpec{opts.palette_a};
    bg.procedural->color_b = AnimatedColorSpec{opts.palette_b};
    bg.procedural->color_c = AnimatedColorSpec{opts.palette_c};
    bg.procedural->speed = AnimatedScalarSpec{opts.speed};
    bg.procedural->frequency = AnimatedScalarSpec{opts.frequency};
    bg.procedural->amplitude = AnimatedScalarSpec{opts.amplitude};
    bg.procedural->scale = AnimatedScalarSpec{opts.scale};
    bg.procedural->grain_amount = AnimatedScalarSpec{opts.grain};
    bg.procedural->contrast = AnimatedScalarSpec{opts.contrast};
    bg.procedural->gamma = AnimatedScalarSpec{1.0};
    bg.procedural->saturation = AnimatedScalarSpec{opts.saturation};
    bg.procedural->softness = AnimatedScalarSpec{opts.softness};
    bg.procedural->octave_decay = AnimatedScalarSpec{opts.octave_decay};
    bg.procedural->band_height = AnimatedScalarSpec{opts.band_height};
    bg.procedural->band_spread = AnimatedScalarSpec{opts.band_spread};
    return bg;
}

/**
 * @brief Create a shape grid procedural background layer.
 */
[[nodiscard]] inline LayerSpec shapegrid(const ShapeGridOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "ShapeGrid Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "grid";
    bg.procedural->seed = opts.seed;
    bg.procedural->color_a = AnimatedColorSpec{opts.palette_a};
    bg.procedural->color_b = AnimatedColorSpec{opts.palette_b};
    bg.procedural->color_c = AnimatedColorSpec{opts.palette_c};
    bg.procedural->speed = AnimatedScalarSpec{opts.speed};
    bg.procedural->frequency = AnimatedScalarSpec{opts.frequency};
    bg.procedural->amplitude = AnimatedScalarSpec{opts.amplitude};
    bg.procedural->scale = AnimatedScalarSpec{opts.scale};
    bg.procedural->shape = opts.shape;
    bg.procedural->spacing = AnimatedScalarSpec{opts.spacing};
    bg.procedural->border_width = AnimatedScalarSpec{opts.border_width};
    bg.procedural->grain_amount = AnimatedScalarSpec{opts.grain};
    bg.procedural->contrast = AnimatedScalarSpec{opts.contrast};
    bg.procedural->gamma = AnimatedScalarSpec{1.0};
    bg.procedural->saturation = AnimatedScalarSpec{opts.saturation};
    bg.procedural->softness = AnimatedScalarSpec{opts.softness};
    return bg;
}

/**
 * @brief Create a stars procedural background layer.
 */
[[nodiscard]] inline LayerSpec stars(const StarsOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "Stars Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "stars";
    bg.procedural->seed = opts.seed;
    bg.procedural->color_a = AnimatedColorSpec{opts.palette_a};
    bg.procedural->color_b = AnimatedColorSpec{opts.palette_b};
    bg.procedural->color_c = AnimatedColorSpec{opts.palette_c};
    bg.procedural->speed = AnimatedScalarSpec{opts.speed};
    bg.procedural->frequency = AnimatedScalarSpec{opts.frequency};
    bg.procedural->amplitude = AnimatedScalarSpec{opts.amplitude};
    bg.procedural->scale = AnimatedScalarSpec{opts.scale};
    bg.procedural->grain_amount = AnimatedScalarSpec{opts.grain};
    bg.procedural->contrast = AnimatedScalarSpec{opts.contrast};
    bg.procedural->gamma = AnimatedScalarSpec{1.0};
    bg.procedural->saturation = AnimatedScalarSpec{opts.saturation};
    bg.procedural->softness = AnimatedScalarSpec{opts.softness};
    return bg;
}

/**
 * @brief Create a noise procedural background layer.
 */
[[nodiscard]] inline LayerSpec noise(const NoiseOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "Noise Background";
    bg.type = "procedural";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    bg.procedural = ProceduralSpec{};
    bg.procedural->kind = "noise";
    bg.procedural->seed = opts.seed;
    bg.procedural->color_a = AnimatedColorSpec{opts.palette_a};
    bg.procedural->color_b = AnimatedColorSpec{opts.palette_b};
    bg.procedural->color_c = AnimatedColorSpec{opts.palette_c};
    bg.procedural->speed = AnimatedScalarSpec{opts.speed};
    bg.procedural->frequency = AnimatedScalarSpec{opts.frequency};
    bg.procedural->amplitude = AnimatedScalarSpec{opts.amplitude};
    bg.procedural->scale = AnimatedScalarSpec{opts.scale};
    bg.procedural->grain_amount = AnimatedScalarSpec{opts.grain};
    bg.procedural->contrast = AnimatedScalarSpec{opts.contrast};
    bg.procedural->gamma = AnimatedScalarSpec{1.0};
    bg.procedural->saturation = AnimatedScalarSpec{opts.saturation};
    bg.procedural->softness = AnimatedScalarSpec{opts.softness};
    return bg;
}

/**
 * @brief Create a solid color background layer.
 */
[[nodiscard]] inline LayerSpec solid(const BackgroundOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "Solid Background";
    bg.type = "solid";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    return bg;
}

/**
 * @brief Create an image background layer.
 */
[[nodiscard]] inline LayerSpec image(std::string path, const BackgroundOptions& opts = {}) {
    LayerSpec bg;
    bg.id = opts.id;
    bg.name = "Image Background";
    bg.type = "image";
    bg.enabled = true;
    bg.visible = true;
    bg.start_time = 0.0;
    bg.in_point = 0.0;
    bg.out_point = opts.duration;
    bg.width = opts.width;
    bg.height = opts.height;
    bg.opacity = 1.0;
    bg.asset_id = std::move(path);
    return bg;
}

/**
 * @brief Set the clear color for a composition.
 * 
 * @code
 * c.clear({13, 17, 23, 255});
 * @endcode
 */
inline void set_clear_color(CompositionSpec& comp, const ColorSpec& color) {
    comp.background = BackgroundSpec{};
    comp.background->type = BackgroundType::Color;
    comp.background->parsed_color = color;
    comp.background->value = "solid_color";
}

/**
 * @brief Create a minimal white background (noise-based with white palette).
 */
[[nodiscard]] inline LayerSpec minimal_white(const BackgroundOptions& opts = {}) {
    NoiseOptions noise_opts;
    noise_opts.id = opts.id;
    noise_opts.width = opts.width;
    noise_opts.height = opts.height;
    noise_opts.duration = opts.duration;
    noise_opts.seed = opts.seed;
    noise_opts.palette_a = {255, 255, 255, 255};
    noise_opts.palette_b = {248, 248, 248, 255};
    noise_opts.palette_c = {240, 240, 240, 0};
    noise_opts.speed = 0.5;
    noise_opts.frequency = 0.0;
    noise_opts.grain = 0.02;
    noise_opts.contrast = 1.0;
    noise_opts.saturation = 0.0;
    noise_opts.softness = 0.3;
    return noise(noise_opts);
}

} // namespace tachyon::presets::background
