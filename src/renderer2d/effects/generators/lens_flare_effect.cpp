#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"

#include <cmath>
#include <vector>
#include <nlohmann/json.hpp>

namespace tachyon::renderer2d {
namespace {

struct LightSource {
    float x, y;       // Normalized position [0, 1]
    float r, g, b;    // Color
    float intensity;    // Intensity multiplier
};

std::vector<LightSource> parse_lights(const std::string& json_str) {
    std::vector<LightSource> lights;
    try {
        auto j = nlohmann::json::parse(json_str);
        if (j.is_array()) {
            for (const auto& light : j) {
                LightSource ls;
                ls.x = light.value("x", 0.5f);
                ls.y = light.value("y", 0.5f);
                ls.r = light.value("r", 1.0f);
                ls.g = light.value("g", 1.0f);
                ls.b = light.value("b", 1.0f);
                ls.intensity = light.value("intensity", 1.0f);
                lights.push_back(ls);
            }
        }
    } catch (...) {
        // Invalid JSON, return empty
    }
    return lights;
}

// Add a simple circular flare element
void add_flare_circle(SurfaceRGBA& out, float cx, float cy, float radius, 
                           float r, float g, float b, float intensity, int width, int height) {
    int rx = static_cast<int>(cx * width);
    int ry = static_cast<int>(cy * height);
    int rr = static_cast<int>(radius * std::min(width, height));
    
    for (int dy = -rr; dy <= rr; ++dy) {
        for (int dx = -rr; dx <= rr; ++dx) {
            int px = rx + dx;
            int py = ry + dy;
            if (px < 0 || px >= width || py < 0 || py >= height) continue;
            
            float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy));
            if (dist > rr) continue;
            
            float alpha = std::max(0.0f, 1.0f - dist/rr) * intensity;
            Color dst = out.get_pixel(static_cast<std::uint32_t>(px), static_cast<std::uint32_t>(py));
            dst.r += r * alpha;
            dst.g += g * alpha;
            dst.b += b * alpha;
            dst.a = std::min(1.0f, dst.a + alpha);
            out.set_pixel(static_cast<std::uint32_t>(px), static_cast<std::uint32_t>(py), dst);
        }
    }
}

// Add a streak (line) flare element
void add_flare_streak(SurfaceRGBA& out, float x1, float y1, float x2, float y2,
                            float r, float g, float b, float intensity, int width, int height) {
    int ix1 = static_cast<int>(x1 * width);
    int iy1 = static_cast<int>(y1 * height);
    int ix2 = static_cast<int>(x2 * width);
    int iy2 = static_cast<int>(y2 * height);
    
    int dx = ix2 - ix1;
    int dy = iy2 - iy1;
    int steps = std::max(std::abs(dx), std::abs(dy)) * 2;
    
    for (int s = 0; s <= steps; ++s) {
        float t = steps > 0 ? s / static_cast<float>(steps) : 0.0f;
        int px = static_cast<int>(ix1 + dx * t);
        int py = static_cast<int>(iy1 + dy * t);
        
        if (px < 0 || px >= width || py < 0 || py >= height) continue;
        
        float alpha = intensity * (1.0f - std::abs(t - 0.5f) * 2.0f);
        alpha = std::max(0.0f, alpha);
        
        Color dst = out.get_pixel(static_cast<std::uint32_t>(px), static_cast<std::uint32_t>(py));
        dst.r += r * alpha;
        dst.g += g * alpha;
        dst.b += b * alpha;
        dst.a = std::min(1.0f, dst.a + alpha);
        out.set_pixel(static_cast<std::uint32_t>(px), static_cast<std::uint32_t>(py), dst);
    }
}

} // namespace

SurfaceRGBA LensFlareEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    SurfaceRGBA out = input;
    
    // Get light sources from parameters
    std::string lights_json;
    auto it = params.strings.find("lights");
    if (it != params.strings.end()) {
        lights_json = it->second;
    }
    
    auto lights = parse_lights(lights_json);
    if (lights.empty()) {
        return out; // No lights, no flare
    }
    
    int width = static_cast<int>(out.width());
    int height = static_cast<int>(out.height());
    float center_x = 0.5f;
    float center_y = 0.5f;
    
    for (const auto& light : lights) {
        // Calculate direction from light to center
        float dir_x = center_x - light.x;
        float dir_y = center_y - light.y;
        float dist = std::sqrt(dir_x * dir_x + dir_y * dir_y);
        if (dist < 0.001f) continue;
        
        // Normalize direction
        dir_x /= dist;
        dir_y /= dist;
        
        // Scale intensity (simple: brighter when closer to center)
        float intensity = light.intensity * (1.0f - std::min(1.0f, dist));
        
        // Generate flare elements along the ray
        
        // 1. Halo at light position
        add_flare_circle(out, light.x, light.y, 0.05f, 
                         light.r, light.g, light.b, intensity * 0.5f, width, height);
        
        // 2. Ghosts along the ray (reflections)
        for (int ghost = 1; ghost <= 3; ++ghost) {
            float t = static_cast<float>(ghost) / 4.0f; // Ghosts between light and center
            float gx = light.x + dir_x * dist * t;
            float gy = light.y + dir_y * dist * t;
            
            // Alternate ghost colors
            float gr = ghost % 2 == 0 ? light.r : light.g;
            float gg = ghost % 2 == 0 ? light.g : light.b;
            float gb = ghost % 2 == 0 ? light.b : light.r;
            
            add_flare_circle(out, gx, gy, 0.02f + 0.01f * ghost,
                             gr, gg, gb, intensity * 0.3f / ghost, width, height);
        }
        
        // 3. Streak along the ray direction
        float streak_start_x = light.x + dir_x * dist * 0.8f;
        float streak_start_y = light.y + dir_y * dist * 0.8f;
        float streak_end_x = light.x + dir_x * dist * 1.2f;
        float streak_end_y = light.y + dir_y * dist * 1.2f;
        
        add_flare_streak(out, streak_start_x, streak_start_y, streak_end_x, streak_end_y,
                        light.r, light.g, light.b, intensity * 0.4f, width, height);
    }
    
    // Clamp output colors
    for (std::uint32_t y = 0; y < out.height(); ++y) {
        for (std::uint32_t x = 0; x < out.width(); ++x) {
            Color px = out.get_pixel(x, y);
            px.r = std::min(1.0f, px.r);
            px.g = std::min(1.0f, px.g);
            px.b = std::min(1.0f, px.b);
            out.set_pixel(x, y, px);
        }
    }
    
    return out;
}

} // namespace tachyon::renderer2d

