#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include <cmath>
#include <algorithm>
#include <functional>

namespace tachyon::renderer2d {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

bool has_scalar(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.scalars.find(key) != params.scalars.end()) {
            return true;
        }
    }
    return false;
}

bool has_color(const EffectParams& params, const std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (params.colors.find(key) != params.colors.end()) {
            return true;
        }
    }
    return false;
}

float get_scalar(const EffectParams& params, const std::string& key, float fallback) {
    const auto it = params.scalars.find(key);
    return it != params.scalars.end() ? static_cast<float>(it->second) : fallback;
}

Color get_color(const EffectParams& params, const std::string& key, Color fallback) {
    const auto it = params.colors.find(key);
    if (it == params.colors.end()) {
        return fallback;
    }
    return Color{it->second.r, it->second.g, it->second.b, it->second.a};
}

static std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

float random01(std::uint64_t seed, std::uint64_t index, std::uint64_t salt) {
    const std::uint64_t mixed = splitmix64(seed ^ (index * 0x9e3779b97f4a7c15ULL) ^ salt);
    return static_cast<float>((mixed >> 11) * (1.0 / 9007199254740992.0));
}

void draw_disk(SurfaceRGBA& surface, int cx, int cy, int radius, Color color) {
    if (radius <= 0) {
        if (cx >= 0 && cy >= 0 && static_cast<std::uint32_t>(cx) < surface.width() && static_cast<std::uint32_t>(cy) < surface.height())
            surface.blend_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), color);
        return;
    }
    const int r2 = radius * radius;
    const int min_x = std::max(0, cx - radius), max_x = std::min(static_cast<int>(surface.width()) - 1, cx + radius);
    const int min_y = std::max(0, cy - radius), max_y = std::min(static_cast<int>(surface.height()) - 1, cy + radius);
    for (int y = min_y; y <= max_y; ++y)
        for (int x = min_x; x <= max_x; ++x)
            if (((x - cx) * (x - cx) + (y - cy) * (y - cy)) <= r2)
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
}

} // namespace tachyon::renderer2d
