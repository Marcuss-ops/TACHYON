#include "tachyon/renderer2d/evaluated_composition/rendering/layer_renderer_procedural.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/math/noise.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace tachyon::renderer2d {

/**
 * @brief Render a procedural layer to a framebuffer.
 * 
 * Generates procedural patterns (noise, waves, stripes, gradient_sweep)
 * based on the ProceduralSpec parameters.
 */
static void render_procedural_pattern(
    Framebuffer& fb,
    const ProceduralSpec& spec,
    double time_seconds) {
    
    math::PerlinNoise noise(spec.seed);
    
    // Sample parameters at current time
    // For simplicity, use the static values; animated values would need sampling
    double frequency = spec.frequency.value.value_or(1.0);
    double speed = spec.speed.value.value_or(1.0);
    double amplitude = spec.amplitude.value.value_or(1.0);
    double scale = spec.scale.value.value_or(1.0);
    double angle = spec.angle.value.value_or(0.0);
    double spacing = spec.spacing.value.value_or(50.0);
    
    // Colors
    ColorSpec color_a_spec = spec.color_a.value.value_or(ColorSpec{255, 0, 0, 255});
    ColorSpec color_b_spec = spec.color_b.value.value_or(ColorSpec{0, 0, 255, 255});
    
    Color color_a{color_a_spec.r / 255.0f, color_a_spec.g / 255.0f, color_a_spec.b / 255.0f, color_a_spec.a / 255.0f};
    Color color_b{color_b_spec.r / 255.0f, color_b_spec.g / 255.0f, color_b_spec.b / 255.0f, color_b_spec.a / 255.0f};

    float t = static_cast<float>(time_seconds * speed);
    float freq = static_cast<float>(frequency);
    float amp = static_cast<float>(amplitude);
    float s = static_cast<float>(scale);
    
    // Convert angle to radians
    float angle_rad = static_cast<float>(angle * 3.141592653589793 / 180.0);
    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);
    
    const uint32_t width = fb.width();
    const uint32_t height = fb.height();

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            float u = x / static_cast<float>(width);
            float v = y / static_cast<float>(height);
            
            float value = 0.0f;
            
            if (spec.kind == "noise") {
                // Noise pattern
                value = noise.noise3d(u * freq * s, v * freq * s, t * freq);
                value *= amp;
                // Normalize from [-1,1] to [0,1]
                value = (value + 1.0f) * 0.5f;
                
            } else if (spec.kind == "waves") {
                // Wave pattern
                float wave_u = u * cos_a - v * sin_a;
                value = std::sin(wave_u * freq * 6.28318530718f + t * 2.0f) * amp;
                // Normalize from [-amp, amp] to [0,1] (approximately)
                value = (value / (amp + 1.0f)) * 0.5f + 0.5f;
                
            } else if (spec.kind == "stripes") {
                // Stripe pattern
                float stripe_u = u * cos_a - v * sin_a;
                float period = spacing > 0.0 ? static_cast<float>(1.0 / spacing) : 0.1f;
                value = std::fmod(stripe_u * period * freq, 1.0f);
                if (value < 0.0f) value += 1.0f;
                value = value < 0.5f ? 0.0f : 1.0f;
                
            } else if (spec.kind == "gradient_sweep") {
                // Gradient sweep (circular)
                float dx = u - 0.5f;
                float dy = v - 0.5f;
                float dist = std::sqrt(dx * dx + dy * dy) * 2.0f; // 0 to ~1.4
                value = std::fmod(dist * freq + t * 0.1f, 1.0f);
                if (value < 0.0f) value += 1.0f;
            }
            
            // Clamp value to [0,1]
            value = std::clamp(value, 0.0f, 1.0f);
            
            // Lerp between color_a and color_b
            Color final_color;
            final_color.r = color_a.r * (1.0f - value) + color_b.r * value;
            final_color.g = color_a.g * (1.0f - value) + color_b.g * value;
            final_color.b = color_a.b * (1.0f - value) + color_b.b * value;
            final_color.a = color_a.a * (1.0f - value) + color_b.a * value;
            
            fb.set_pixel(x, y, final_color);
        }
    }
}

/**
 * @brief Render a procedural layer.
 * 
 * @param fb Framebuffer to render to
 * @param evaluated The evaluated layer state
 * @param time_seconds Current time in seconds
 */
void render_procedural_layer(
    Framebuffer& fb,
    const tachyon::scene::EvaluatedLayerState& evaluated,
    double time_seconds) {
    
    if (!evaluated.procedural.has_value()) return;
    render_procedural_pattern(fb, *evaluated.procedural, time_seconds);
}

} // namespace tachyon::renderer2d
