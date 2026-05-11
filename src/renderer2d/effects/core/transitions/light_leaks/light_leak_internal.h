#pragma once
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"

namespace tachyon::renderer2d {

struct LightLeakStyle {
    const char* id;
    const char* name;
    const char* description;

    Color color_a;
    Color color_b;
    Color highlight;

    float angle_degrees;
    float width;
    float softness;
    float intensity;
    float speed;
    float offset;
    float direction; // -1 for left/top, 1 for right/bottom

    float flicker_amount;
    float pulse_amount;

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

} // namespace tachyon::renderer2d
