#include "light_leak_internal.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

float smoothstep01(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float radians(float degrees) {
    return degrees * 3.1415926535f / 180.0f;
}

float soft_band(float value, float center, float width, float softness) {
    const float d = std::abs(value - center);
    float x = 1.0f - std::clamp(d / std::max(width, 0.0001f), 0.0f, 1.0f);
    x = smoothstep01(x);
    return std::pow(x, std::max(softness, 0.1f));
}

float edge_mask(float u, float v, float t, const LightLeakStyle& s) {
    float side = s.direction < 0 ? u : 1.0f - u;
    float drift = 0.08f * std::sin(t * 3.0f + v * 2.0f);
    return std::pow(1.0f - std::clamp((side + drift) / s.width, 0.0f, 1.0f), s.softness);
}

float sweep_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a = radians(s.angle_degrees);
    float proj = u * std::cos(a) + v * std::sin(a);
    float center = -s.offset + t * s.speed;
    return soft_band(proj, center, s.width, s.softness);
}

float radial_corner_mask(float u, float v, float t, const LightLeakStyle& s) {
    float dx = (s.direction < 0) ? u : 1.0f - u;
    float dy = (s.direction < 0) ? v : 1.0f - v;
    float d = std::sqrt(dx * dx + dy * dy);
    float pulse = 1.0f + s.pulse_amount * std::sin(t * 3.14159f);
    return std::pow(1.0f - std::clamp(d / (s.width * pulse), 0.0f, 1.0f), s.softness);
}

float horizontal_band_mask(float u, float v, float t, const LightLeakStyle& s) {
    float center = 0.5f + (s.direction * 0.1f) * std::sin(t * 2.0f);
    return soft_band(v, center, s.width, s.softness);
}

float window_bands_mask(float u, float v, float t, const LightLeakStyle& s) {
    float drift = 0.03f * std::sin(t * 2.0f);
    float m1 = soft_band(u, 0.20f + drift, s.width, s.softness);
    float m2 = soft_band(u, 0.48f + drift, s.width, s.softness);
    float m3 = soft_band(u, 0.76f + drift, s.width, s.softness);
    return std::clamp(m1 + m2 * 0.7f + m3 * 0.5f, 0.0f, 1.0f);
}

float dual_beam_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a1 = radians(s.angle_degrees);
    float a2 = radians(s.angle_degrees + 20.0f);
    float proj1 = u * std::cos(a1) + v * std::sin(a1);
    float proj2 = u * std::cos(a2) - v * std::sin(a2);
    float center = -s.offset + t * s.speed;
    return 0.7f * soft_band(proj1, center, s.width, s.softness) + 
           0.5f * soft_band(proj2, center * 0.9f, s.width * 0.8f, s.softness);
}

float prism_mask(float u, float v, float t, const LightLeakStyle& s) {
    float proj = u * 0.8f + v * 0.2f;
    float center = -s.offset + t * s.speed;
    float m1 = soft_band(proj, center - 0.05f, s.width * 0.5f, s.softness);
    float m2 = soft_band(proj, center,         s.width * 0.5f, s.softness);
    float m3 = soft_band(proj, center + 0.05f, s.width * 0.5f, s.softness);
    return std::clamp(m1 + m2 + m3, 0.0f, 1.0f);
}

float blobs_mask(float u, float v, float t, const LightLeakStyle& s) {
    float eased = t * t * (3.0f - 2.0f * t);
    float cx = -0.2f + eased * 1.4f; 
    float cy = 0.5f + std::sin(t * 5.0f) * 0.1f; 

    float dx = u - cx;
    float dy = (v - cy) * 0.5625f; 
    float dist = std::sqrt(dx * dx + dy * dy);
    
    float angle = (dist > 1e-6f) ? std::atan2(dy, dx) : 0.0f;
    float time_scalar = t * 10.0f;
    float noise = std::sin(angle * 3.0f + time_scalar * 1.2f) * 0.012f +
                  std::sin(angle * 2.0f - time_scalar * 0.9f) * 0.008f +
                  std::sin(angle * 5.0f + time_scalar * 0.5f) * 0.004f;

    float base_radius = 0.05f; 
    float actual_radius = base_radius + noise;

    float core = 1.0f - smoothstep01(std::clamp(dist / actual_radius, 0.0f, 1.0f));
    float glow = 0.7f * std::exp(-(dist * dist) / 0.015f); 

    return std::clamp(core + glow, 0.0f, 1.0f);
}

float lava_flow_mask(float u, float v, float t, const LightLeakStyle& s) {
    float sum = 0.0f;
    auto blob = [&](float cx, float cy, float rad) {
        float dx = u - cx;
        float dy = (v - cy) * 0.5625f;
        sum += std::exp(-(dx * dx + dy * dy) / (rad * rad));
    };
    float progression = -0.25f + t * 1.5f; 
    blob(0.20f + 0.06f * std::sin(t * 1.9f), 1.35f - progression * 1.6f, 0.17f);
    blob(0.38f + 0.08f * std::cos(t * 2.1f), 1.50f - progression * 1.8f, 0.14f);
    blob(0.50f + 0.03f * std::sin(t * 3.2f), 1.25f - progression * 1.5f, 0.21f);
    blob(0.65f + 0.09f * std::sin(t * 2.6f), 1.60f - progression * 1.9f, 0.13f);
    blob(0.82f + 0.05f * std::cos(t * 2.3f), 1.45f - progression * 1.7f, 0.18f);
    if (t > 0.20f && t < 0.80f) {
         float dist_from_center = std::abs(t - 0.5f);
         float density_boost = 1.0f - (dist_from_center / 0.30f);
         blob(0.5f, 0.5f, 0.24f * std::max(0.0f, density_boost));
    }
    float core = smoothstep01((sum - 0.36f) / 0.16f);
    float glow = sum * 0.40f;
    return std::clamp(core + glow, 0.0f, 1.0f);
}

float liquid_fission_mask(float u, float v, float t, const LightLeakStyle& s) {
    float dist_mapped = std::abs(t - 0.5f) * 2.0f; 
    float offset = std::pow(dist_mapped, 1.7f) * 1.3f; 
    float interaction = 1.0f - dist_mapped;
    interaction = std::clamp(interaction, 0.0f, 1.0f);
    float base_rad = 0.14f;
    float fusion_rad = base_rad + 0.25f * smoothstep01(interaction);
    float sum = 0.0f;
    auto blob = [&](float cx, float cy, float rad) {
        float dx = u - cx;
        float dy = (v - cy) * 0.5625f;
        sum += std::exp(-(dx * dx + dy * dy) / (rad * rad));
    };
    blob(0.5f - offset, 0.5f, fusion_rad);
    blob(0.5f + offset, 0.5f, fusion_rad);
    float angle = std::atan2((v - 0.5f) * 0.5625f, u - 0.5f);
    float turbulence = 0.03f * std::sin(angle * 5.0f + t * 6.0f) * smoothstep01(interaction);
    float core = smoothstep01((sum - 0.33f + turbulence) / 0.18f);
    float glow = sum * 0.50f;
    return std::clamp(core + glow, 0.0f, 1.0f);
}

float cosmic_swirl_mask(float u, float v, float t, const LightLeakStyle& s) {
    float sum = 0.0f;
    auto blob = [&](float cx, float cy, float rad) {
        float dx = u - cx;
        float dy = (v - cy) * 0.5625f;
        sum += std::exp(-(dx * dx + dy * dy) / (rad * rad));
    };
    float angle_base = t * 5.5f; 
    float dist_mapped = std::abs(t - 0.5f) * 2.0f; 
    for (int i = 0; i < 3; ++i) {
        float phase = (i * 2.0944f) + angle_base;
        float orbit_rad = 0.1f + 0.5f * std::pow(dist_mapped, 1.2f);
        float cx = 0.5f + std::cos(phase) * orbit_rad;
        float cy = 0.5f + std::sin(phase) * orbit_rad;
        float local_rad = 0.11f + 0.18f * (1.0f - smoothstep01(dist_mapped));
        blob(cx, cy, local_rad);
    }
    if (dist_mapped < 0.35f) {
         float magnetism = 1.0f - (dist_mapped / 0.35f);
         blob(0.5f, 0.5f, 0.28f * smoothstep01(magnetism));
    }
    float core = smoothstep01((sum - 0.40f) / 0.18f);
    float glow = sum * 0.60f;
    return std::clamp(core + glow, 0.0f, 1.0f);
}

float cinematic_amber_mask(float u, float v, float t, const LightLeakStyle& s) {
    float sum = 0.0f;
    auto blob = [&](float cx, float cy, float rad) {
        float dx = u - cx;
        float dy = (v - cy) * 0.5625f;
        sum += std::exp(-(dx * dx + dy * dy) / (rad * rad));
    };
    
    // Tuned radii applied from the earlier optimized fix
    float slow_t = t * 0.75f;
    blob(0.5f + 0.35f * std::cos(slow_t * 1.2f), 0.5f + 0.25f * std::sin(slow_t * 1.5f), 0.28f);
    blob(0.3f + 0.25f * std::sin(slow_t * 1.4f), 0.7f - 0.35f * std::cos(slow_t * 1.1f), 0.24f);
    blob(0.8f - 0.45f * std::cos(slow_t * 1.3f), 0.2f + 0.35f * std::sin(slow_t * 1.7f), 0.26f);
    
    float angle = std::atan2((v - 0.5f) * 0.5625f, u - 0.5f);
    float flow = 0.025f * std::sin(angle * 3.0f + t * 2.2f);
    
    float core = smoothstep01((sum - 0.45f + flow) / 0.25f);
    float glow = sum * 0.60f;
    return std::clamp(core + glow, 0.0f, 1.0f);
}

// Fast procedural hash noise exactly matching the requested GLSL algorithm
inline float noise_hash(float x, float y) {
    float val = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
    return val - std::floor(val);
}

// Efficient 2D smooth noise bilerp CPU implementation
float value_noise_2d(float x, float y) {
    float ix = std::floor(x);
    float iy = std::floor(y);
    float fx = x - ix;
    float fy = y - iy;

    float a = noise_hash(ix, iy);
    float b = noise_hash(ix + 1.0f, iy);
    float c = noise_hash(ix, iy + 1.0f);
    float d = noise_hash(ix + 1.0f, iy + 1.0f);

    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);

    return (a * (1.0f - ux) + b * ux) * (1.0f - uy) + 
           (c * (1.0f - ux) + d * ux) * uy;
}

float procedural_remotion_mask(float u, float v, float t, const LightLeakStyle& s) {
    // Reveal / Retract curve: peak exactly at the transition midpoint
    float p = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
    p = smoothstep01(p);

    // Replicate the exact 3-octave composite noise with directional drifts
    // Using s.direction as the 'seed' input
    float seed = s.direction * 100.0f;
    
    float n1 = value_noise_2d(u * 2.5f + seed, v * 2.5f + t * 0.3f);
    float n2 = value_noise_2d(u * 4.0f - seed * 1.3f, v * 4.0f + t * 0.5f);
    float n3 = value_noise_2d(u * 1.7f + seed * 0.5f, v * 1.7f + t * 1.0f);

    // Combined non-linear leak density exactly as requested
    float leak = std::pow(n1 * 0.6f + n2 * 0.3f + n3 * 0.1f, 2.0f);

    // Radial distance vignette (normalization account for widescreen 16:9)
    float dx = u - 0.5f;
    float dy = (v - 0.5f) * 0.5625f;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Remotion edge logic: light enters and blooms from edges
    // Remap dist into [0..1] approximate and smoothstep
    float edge_factor = std::clamp((dist - 0.2f) / 0.6f, 0.0f, 1.0f);
    float vignette = edge_factor * edge_factor * (3.0f - 2.0f * edge_factor);

    // High concentration multiplication + apply envelope
    return leak * vignette * p * 2.0f; // 2.0x boost for intensity alignment
}

} // namespace

float evaluate_light_leak_mask(float u, float v, float t, const LightLeakStyle& style) {
    switch (style.shape) {
        case LightLeakStyle::Shape::Edge:           return edge_mask(u, v, t, style);
        case LightLeakStyle::Shape::Sweep:          return sweep_mask(u, v, t, style);
        case LightLeakStyle::Shape::RadialCorner:   return radial_corner_mask(u, v, t, style);
        case LightLeakStyle::Shape::HorizontalBand: return horizontal_band_mask(u, v, t, style);
        case LightLeakStyle::Shape::WindowBands:    return window_bands_mask(u, v, t, style);
        case LightLeakStyle::Shape::DualBeam:       return dual_beam_mask(u, v, t, style);
        case LightLeakStyle::Shape::Prism:          return prism_mask(u, v, t, style);
        case LightLeakStyle::Shape::Blobs:          return blobs_mask(u, v, t, style);
        case LightLeakStyle::Shape::LavaFlow:       return lava_flow_mask(u, v, t, style);
        case LightLeakStyle::Shape::LiquidFission:  return liquid_fission_mask(u, v, t, style);
        case LightLeakStyle::Shape::CosmicSwirl:    return cosmic_swirl_mask(u, v, t, style);
        case LightLeakStyle::Shape::CinematicAmber: return cinematic_amber_mask(u, v, t, style);
        case LightLeakStyle::Shape::ProceduralRemotion: return procedural_remotion_mask(u, v, t, style);
        default: return 0.0f;
    }
}

} // namespace tachyon::renderer2d
