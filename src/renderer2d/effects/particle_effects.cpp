#include "tachyon/renderer2d/effects/effect_common.h"

namespace tachyon::renderer2d {

namespace {

std::uint64_t splitmix64(std::uint64_t x) {
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
        if (cx >= 0 && cy >= 0 && static_cast<std::uint32_t>(cx) < surface.width() && static_cast<std::uint32_t>(cy) < surface.height()) {
            surface.blend_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), color);
        }
        return;
    }

    const int r2 = radius * radius;
    const int min_x = std::max(0, cx - radius);
    const int max_x = std::min(static_cast<int>(surface.width()) - 1, cx + radius);
    const int min_y = std::max(0, cy - radius);
    const int max_y = std::min(static_cast<int>(surface.height()) - 1, cy + radius);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;
            if ((dx * dx + dy * dy) <= r2) {
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
            }
        }
    }
}

} // namespace

SurfaceRGBA ParticleEmitterEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    if (input.width() == 0U || input.height() == 0U) {
        return input;
    }

    const std::uint64_t seed = static_cast<std::uint64_t>(std::llround(get_scalar(params, "seed", 1.0f)));
    const std::size_t count = std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(get_scalar(params, "count", 48.0f))));
    const float time = get_scalar(params, "time", 0.0f);
    const float lifetime = std::max(0.1f, get_scalar(params, "lifetime", 1.0f));
    const float speed = get_scalar(params, "speed", 18.0f);
    const float gravity = get_scalar(params, "gravity", 0.0f);
    const float spread_x = get_scalar(params, "spread_x", 1.0f);
    const float spread_y = get_scalar(params, "spread_y", 1.0f);
    const float radius_min = std::max(0.0f, get_scalar(params, "radius_min", 1.0f));
    const float radius_max = std::max(radius_min, get_scalar(params, "radius_max", 3.0f));
    const Color particle_color = get_color(params, "color", Color{1.0f, 1.0f, 1.0f, 0.65f});
    const float opacity = clamp01(get_scalar(params, "opacity", 1.0f));
    const float cx = get_scalar(params, "center_x", 0.5f) * static_cast<float>(input.width());
    const float cy = get_scalar(params, "center_y", 0.5f) * static_cast<float>(input.height());
    const float emit_w = std::max(1.0f, get_scalar(params, "emit_width", static_cast<float>(input.width())));
    const float emit_h = std::max(1.0f, get_scalar(params, "emit_height", static_cast<float>(input.height())));

    SurfaceRGBA out = input;
    for (std::size_t i = 0; i < count; ++i) {
        const float phase = random01(seed, i, 0x1a2b3c4dULL);
        const float angle = random01(seed, i, 0x9e3779b9ULL) * 6.28318530718f;
        const float radius = random01(seed, i, 0xdeadbeefULL) * 0.5f + 0.5f;
        const float life_phase = std::fmod(phase + time / lifetime, 1.0f);
        const float age = life_phase * lifetime;
        const float speed_scale = speed * (0.35f + 0.65f * random01(seed, i, 0x13579bdfULL));

        const float start_x = cx + (random01(seed, i, 0x2468ace0ULL) - 0.5f) * emit_w * spread_x;
        const float start_y = cy + (random01(seed, i, 0xabcdef01ULL) - 0.5f) * emit_h * spread_y;
        const float vx = std::cos(angle) * speed_scale * (0.25f + radius);
        const float vy = std::sin(angle) * speed_scale * (0.25f + radius);

        const float px = start_x + vx * age * 0.05f;
        const float py = start_y + vy * age * 0.05f + 0.5f * gravity * age * age;
        const int draw_radius = static_cast<int>(std::lround(radius_min + (radius_max - radius_min) * radius));

        Color c = particle_color;
        const float fade_in = std::min(1.0f, age / (lifetime * 0.15f));
        const float fade_out = std::min(1.0f, (lifetime - age) / (lifetime * 0.35f));
        c.a *= opacity * std::clamp(fade_in * fade_out, 0.0f, 1.0f);
        if (c.a <= 0.0f) {
            continue;
        }

        draw_disk(out, static_cast<int>(std::lround(px)), static_cast<int>(std::lround(py)), draw_radius, c);
    }

    return out;
}

} // namespace tachyon::renderer2d
