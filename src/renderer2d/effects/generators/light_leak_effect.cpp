#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_param_access.h"
#include "tachyon/renderer2d/effects/effect_common.h"
#include "light_leak_presets.h"

#include <random>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace tachyon::renderer2d {

SurfaceRGBA LightLeakEffect::apply(const SurfaceRGBA& input, const EffectParams& p) const {
    const uint32_t w = input.width();
    const uint32_t h = input.height();
    if (w == 0 || h == 0) return input;

    // Parametri base
    float progress = get_scalar(p, "progress", 0.0f);
    const float speed = get_scalar(p, "speed", 1.0f);
    const int seed = static_cast<int>(get_scalar(p, "seed", 3.0f));
    const LightLeakPreset preset_type = static_cast<LightLeakPreset>(static_cast<int>(get_scalar(p, "preset", 0.0f)));
    
    // Applica speed al progress
    progress *= speed;
    if (progress <= 0.001f || progress >= 0.999f) {
        return input;
    }

    // Ottieni impostazioni dal preset
    LightLeakSettings s = make_light_leak_preset(preset_type);
    
    // Override manuali da EffectParams (se presenti)
    if (has_scalar(p, {"intensity"})) s.intensity = get_scalar(p, "intensity", s.intensity);
    if (has_scalar(p, {"width"})) s.width = get_scalar(p, "width", s.width);
    if (has_color(p, {"color_a"})) s.colorA = get_color(p, "color_a", s.colorA);
    if (has_color(p, {"color_b"})) s.colorB = get_color(p, "color_b", s.colorB);
    
    // Calcolo angolo
    float angle = s.angle;
    if (angle == 0.0f) {
        std::mt19937 rng(static_cast<unsigned int>(seed));
        std::uniform_real_distribution<float> angDist(20.0f, 50.0f);
        angle = angDist(rng) * (seed % 2 == 0 ? 1.0f : -1.0f);
    }
    const float rad = angle * M_PI / 180.0f;
    
    // Posizione sweep
    float pos = -0.3f + progress * 1.6f + s.posOffset;
    if (preset_type == LightLeakPreset::WarmFlashLeak) {
        // smoothstep: progress^2 * (3 - 2*progress)
        const float p2 = progress * progress * (3.0f - 2.0f * progress);
        pos = -0.45f + p2 * 1.9f;
    }
    pos += 0.02f * std::sin(progress * 2.0f * M_PI); // micro oscillazione
    
    // Render procedurale
    SurfaceRGBA leak(w, h);
    leak.set_profile(input.profile());
    const float cosA = std::cos(rad);
    const float sinA = std::sin(rad);
    
    // Pre-calcola proiezione normalizzata
    float minProj = 0, maxProj = static_cast<float>(w) * cosA + static_cast<float>(h) * sinA;
    if (maxProj < minProj) std::swap(minProj, maxProj);
    const float range = std::max(1e-6f, maxProj - minProj);
    
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const float fx = static_cast<float>(x);
            const float fy = static_cast<float>(y);
            const float proj = (fx * cosA + fy * sinA - minProj) / range; // 0..1
            
            // Mask gaussiana base
            const float d = proj - pos;
            float mask = std::exp(-(d * d) / (2.0f * s.width * s.width));
            
            // Varianti preset: double band
            if (s.doubleBand) {
                const float d2 = proj - (pos - 0.16f);
                const float m2 = std::exp(-(d2 * d2) / (2.0f * (s.width * 0.38f) * (s.width * 0.38f)));
                mask = mask * 0.75f + m2 * 0.45f;
            }
            
            // Varianti preset: hot core
            if (s.hotCore) {
                const float core = std::exp(-(d * d) / (2.0f * (s.width * 0.22f) * (s.width * 0.22f)));
                mask = mask * 0.55f + core * 0.75f;
            }
            
            // Intensità (con flicker vintage)
            float inten = s.intensity;
            if (s.vintage) {
                const float flicker = 0.92f + 0.08f * std::sin(progress * 17.0f);
                inten *= flicker;
            }
            
            // Gradiente colore
            const float t = std::clamp(proj, 0.0f, 1.0f);
            const float r = s.colorA.r * (1.0f - t) + s.colorB.r * t;
            const float g = s.colorA.g * (1.0f - t) + s.colorB.g * t;
            const float b = s.colorA.b * (1.0f - t) + s.colorB.b * t;
            
            leak.set_pixel(x, y, Color{ r * mask * inten,
                                        g * mask * inten,
                                        b * mask * inten, 1.0f });
        }
    }
    
    // Rumore vintage (opzionale)
    if (s.vintage) {
        std::mt19937 rng(static_cast<unsigned int>(seed + 42));
        std::uniform_int_distribution<int> distX(0, static_cast<int>(w) - 1);
        std::uniform_int_distribution<int> distY(0, static_cast<int>(h) - 1);
        std::uniform_real_distribution<float> distN(-0.02f, 0.02f);
        
        const int noise_pixels = static_cast<int>(w * h * 0.05f);
        for (int i = 0; i < noise_pixels; ++i) {
            const uint32_t nx = static_cast<uint32_t>(distX(rng));
            const uint32_t ny = static_cast<uint32_t>(distY(rng));
            Color px = leak.get_pixel(nx, ny);
            const float n = distN(rng);
            px.r = std::clamp(px.r + n, 0.0f, 1.0f);
            px.g = std::clamp(px.g + n, 0.0f, 1.0f);
            px.b = std::clamp(px.b + n, 0.0f, 1.0f);
            leak.set_pixel(nx, ny, px);
        }
    }
    
    // Blur finale
    SurfaceRGBA blurredLeak = blur_surface(leak, s.blur);
    
    // Blend Screen
    SurfaceRGBA out(w, h);
    out.set_profile(input.profile());
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const Color base = input.get_pixel(x, y);
            const Color add = blurredLeak.get_pixel(x, y);
            
            Color res;
            res.r = 1.0f - (1.0f - base.r) * (1.0f - add.r);
            res.g = 1.0f - (1.0f - base.g) * (1.0f - add.g);
            res.b = 1.0f - (1.0f - base.b) * (1.0f - add.b);
            res.a = 1.0f;
            
            out.set_pixel(x, y, res);
        }
    }
    
    return out;
}

} // namespace tachyon::renderer2d
