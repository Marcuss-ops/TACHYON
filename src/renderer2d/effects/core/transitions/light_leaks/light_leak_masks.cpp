#include "light_leak_internal.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

float soft_band(float value, float center, float width, float softness) {
    const float d = std::abs(value - center);
    float x = 1.0f - std::clamp(d / std::max(width, 0.0001f), 0.0f, 1.0f);
    x = smoothstep01(x);
    return std::pow(x, std::max(softness, 0.1f));
}

float edge_mask(float u, float v, float t, const LightLeakStyle& s) {
    float side = s.direction < 0 ? u : 1.0f - u;
    float drift = 0.08f * std::sin(t * 3.0f + v * 2.0f);
    return std::pow(1.0f - std::clamp((side + drift) / s.spread, 0.0f, 1.0f), s.softness);
}

float sweep_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a = radians(s.angle);
    float proj = u * std::cos(a) + v * std::sin(a);
    float center = -s.offset + t * s.speed;
    return soft_band(proj, center, s.spread, s.softness);
}

float radial_corner_mask(float u, float v, float t, const LightLeakStyle& s) {
    float dx = (s.direction < 0) ? u : 1.0f - u;
    float dy = (s.direction < 0) ? v : 1.0f - v;
    float d = std::sqrt(dx * dx + dy * dy);
    float pulse = 1.0f + s.pulse * std::sin(t * 3.14159f);
    return std::pow(1.0f - std::clamp(d / (s.spread * pulse), 0.0f, 1.0f), s.softness);
}

float horizontal_band_mask(float u, float v, float t, const LightLeakStyle& s) {
    float center = 0.5f + (s.direction * 0.1f) * std::sin(t * 2.0f);
    return soft_band(v, center, s.spread, s.softness);
}

float window_bands_mask(float u, float v, float t, const LightLeakStyle& s) {
    float drift = 0.03f * std::sin(t * 2.0f);
    float m1 = soft_band(u, 0.20f + drift, s.spread, s.softness);
    float m2 = soft_band(u, 0.48f + drift, s.spread, s.softness);
    float m3 = soft_band(u, 0.76f + drift, s.spread, s.softness);
    return std::clamp(m1 + m2 * 0.7f + m3 * 0.5f, 0.0f, 1.0f);
}

float dual_beam_mask(float u, float v, float t, const LightLeakStyle& s) {
    float a1 = radians(s.angle);
    float a2 = radians(s.angle + 20.0f);
    float proj1 = u * std::cos(a1) + v * std::sin(a1);
    float proj2 = u * std::cos(a2) - v * std::sin(a2);
    float center = -s.offset + t * s.speed;
    return 0.7f * soft_band(proj1, center, s.spread, s.softness) + 
           0.5f * soft_band(proj2, center * 0.9f, s.spread * 0.8f, s.softness);
}

float prism_mask(float u, float v, float t, const LightLeakStyle& s) {
    float proj = u * 0.8f + v * 0.2f;
    float center = -s.offset + t * s.speed;
    float m1 = soft_band(proj, center - 0.05f, s.spread * 0.5f, s.softness);
    float m2 = soft_band(proj, center,         s.spread * 0.5f, s.softness);
    float m3 = soft_band(proj, center + 0.05f, s.spread * 0.5f, s.softness);
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
    // Power of 1.3 provides a smoother, more cinematic start than 0.8 or 1.1
    float p = (t < 0.5f) ? std::pow(t * 2.0f, 1.3f) : ((1.0f - t) * 2.0f);
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
    // Reduced dead-zone (0.05f) to ensure light hits the center earlier
    float edge_factor = std::clamp((dist - 0.05f) / 0.6f, 0.0f, 1.0f);
    float vignette = edge_factor * edge_factor * (3.0f - 2.0f * edge_factor);

    // High concentration multiplication + apply envelope
    return leak * vignette * p * 2.0f; // 2.0x boost for intensity alignment
}

// Exact Remotion shader pattern: first-half reveal, second-half retract.
static void remotion_pattern(float u, float v, float seed, float t,
                             float& out_brightness, float& out_blend, float& out_pattern) {
    float x = u * 0.8f;
    float y = v * 0.8f;
    x += std::sin(seed * 1.61803f) * 5.0f;
    y += std::cos(seed * 2.71828f) * 5.0f;

    for (int i = 1; i < 5; ++i) {
        float fi = static_cast<float>(i);
        float phase = seed * 0.7f * fi;
        float nx = x + (0.6f / fi) * std::cos(fi * y + t * 0.7f + 0.3f * fi + phase) + 20.0f;
        float ny = y + (0.6f / fi) * std::cos(fi * x + t * 0.7f + 0.3f * static_cast<float>(i + 10) + phase) - 20.0f + 15.0f;
        x = nx;
        y = ny;
    }

    float v1 = 0.5f * std::sin(2.0f * x) + 0.5f;
    float v2 = 0.5f * std::sin(2.0f * y) + 0.5f;
    out_blend = std::sin(x + y) * 0.5f + 0.5f;
    out_brightness = v1 * 0.5f + v2 * 0.5f;
    out_pattern = out_brightness * 0.6f + out_blend * 0.4f;
}

float remotion_mask(float u, float v, float t, const LightLeakStyle& s) {
    float aspect = 0.5625f;
    float refScale = 1.92f;
    float motion_scale = std::max(0.1f, s.speed);

    float speed_factor = 1.5f * motion_scale; 
    // Use a power curve for the reveal to make it less "direct"
    float evolve_progress = std::clamp(t * speed_factor, 0.0f, 1.0f);
    float evolve = std::pow(evolve_progress, 1.4f);
    evolve = smoothstep01(evolve);

    float retract = smoothstep01(std::clamp(t * speed_factor - 1.2f, 0.0f, 1.0f));

    float su = u * refScale;
    float sv = v * refScale * aspect;

    float b1, bl1, pv1;
    remotion_pattern(su, sv, s.direction, evolve * 3.14159265f * motion_scale, b1, bl1, pv1);

    float threshA = 1.0f - evolve;
    float revealAlpha = std::clamp((pv1 - threshA) / 0.3f, 0.0f, 1.0f);
    revealAlpha = revealAlpha * revealAlpha * (3.0f - 2.0f * revealAlpha);

    float maxU = refScale;
    float maxV = refScale * aspect;
    float ru = maxU - su;
    float rv = maxV - sv;

    float b2, bl2, pv2;
    remotion_pattern(ru, rv, s.direction + 42.0f, retract * 3.14159265f * motion_scale, b2, bl2, pv2);

    float threshB = 1.0f - retract;
    float eraseAlpha = std::clamp((pv2 - threshB) / 0.3f, 0.0f, 1.0f);
    eraseAlpha = eraseAlpha * eraseAlpha * (3.0f - 2.0f * eraseAlpha);

    float mask = std::clamp(revealAlpha * (1.0f - eraseAlpha), 0.0f, 1.0f);
    
    // Balanced fade: visible at frame 5 (t=0.033 -> fade ~0.1) but gradual until frame 75
    float global_fade = std::pow(std::clamp(t * 2.0f, 0.0f, 1.0f), 1.2f); 
    return mask * global_fade;
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
        case LightLeakStyle::Shape::ProceduralRemotion: return procedural_remotion_mask(u, v, t, style);
        case LightLeakStyle::Shape::Remotion:       return remotion_mask(u, v, t, style);
        default: return 0.0f;
    }
}

void evaluate_remotion_color(float u, float v, float t, const LightLeakStyle& style,
                              float& out_r, float& out_g, float& out_b) {
    float aspect = 0.5625f;
    float refScale = 1.92f;
    float motion_scale = std::max(0.1f, style.speed);
    float evolve = std::min(1.0f, t * 2.4f * motion_scale);

    float su = u * refScale;
    float sv = v * refScale * aspect;

    float brightness, blend, pattern;
    remotion_pattern(su, sv, style.direction, evolve * 3.14159265f * motion_scale, brightness, blend, pattern);

    float base_r = 1.0f;
    float base_g = 0.93f + (0.62f - 0.93f) * blend;
    float base_b = 0.12f + (0.03f - 0.12f) * blend;

    float brightness_factor = 0.65f + 0.55f * brightness;
    base_r *= brightness_factor;
    base_g *= brightness_factor;
    base_b *= brightness_factor;

    float angle = style.angle * 3.14159265f / 180.0f;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float inv3 = 1.0f / 3.0f;
    float sqrt3 = 0.57735f;

    float m00 = cosA + (1.0f - cosA) * inv3;
    float m01 = (1.0f - cosA) * inv3 - sinA * sqrt3;
    float m02 = (1.0f - cosA) * inv3 + sinA * sqrt3;
    float m10 = (1.0f - cosA) * inv3 + sinA * sqrt3;
    float m11 = cosA + (1.0f - cosA) * inv3;
    float m12 = (1.0f - cosA) * inv3 - sinA * sqrt3;
    float m20 = (1.0f - cosA) * inv3 - sinA * sqrt3;
    float m21 = (1.0f - cosA) * inv3 + sinA * sqrt3;
    float m22 = cosA + (1.0f - cosA) * inv3;

    out_r = std::clamp(m00 * base_r + m01 * base_g + m02 * base_b, 0.0f, 1.0f);
    out_g = std::clamp(m10 * base_r + m11 * base_g + m12 * base_b, 0.0f, 1.0f);
    out_b = std::clamp(m20 * base_r + m21 * base_g + m22 * base_b, 0.0f, 1.0f);
}

} // namespace tachyon::renderer2d
