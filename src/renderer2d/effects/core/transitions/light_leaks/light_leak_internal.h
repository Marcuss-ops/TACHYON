#pragma once
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"

namespace tachyon::renderer2d {

struct LightLeakStyle {
    const char* id;
    const char* name;
    const char* description;

    Color inner_color;
    Color mid_color;
    Color outer_color;

    float angle;
    float spread;
    float softness;
    float intensity;
    float speed;
    float offset;
    float direction; 

    float grain;
    float pulse;

    enum class Shape {
        Edge,
        Sweep,
        RadialCorner,
        HorizontalBand,
        WindowBands,
        DualBeam,
        Prism,
        Blobs,
        LavaFlow,
        LiquidFission,
        CosmicSwirl,
        CinematicAmber,
        ProceduralRemotion
    } shape;
};

// --- Forward declarations for the segmented sub-modules ---

// From light_leak_masks.cpp
float evaluate_light_leak_mask(float u, float v, float t, const LightLeakStyle& style);

// From light_leak_engine.cpp
Color apply_light_leak_style(
    float u,
    float v,
    float t,
    const SurfaceRGBA& input,
    const SurfaceRGBA* to_surface,
    const LightLeakStyle& style
);

void apply_light_leak_batch(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    const LightLeakStyle& style,
    int thread_count
);

} // namespace tachyon::renderer2d
