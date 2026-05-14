#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <array>
#include <string_view>

namespace tachyon::renderer2d {

namespace {

Color transition_zoom_in(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    const Color source = sample_uv(input, u, v);
    const Color target = sample_transition_target(input, to_surface, su, sv);
    return Color::lerp(source, target, t);
}

} // namespace

Color transition_zoom_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    Color acc = Color::transparent();
    constexpr int samples = 6;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float offset = (s - 0.5f) * t * 0.12f;
        acc = Color::lerp(acc, sample_transition_target(input, to_surface, su + offset, sv + offset), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(sample_uv(input, u, v), acc, t);
}

namespace {

Color transition_spin(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float angle = t * 6.28318530718f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float ru = 0.5f + dx * c - dy * s;
    const float rv = 0.5f + dx * s + dy * c;
    const Color source = sample_uv(input, ru, rv);
    const Color target = sample_transition_target(input, to_surface, ru, rv);
    return Color::lerp(source, target, t);
}



Color transition_directional_blur_wipe(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float slide = std::clamp(t * 1.5f, 0.0f, 1.0f);
    const float sigma = 0.01f + 0.04f * t;
    Color acc = Color::transparent();
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float off = (s - 0.5f) * sigma;
        acc = Color::lerp(acc, sample_transition_target(input, to_surface, u + slide + off, v), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(sample_uv(input, u, v), acc, t);
}

} // namespace

Color transition_flash(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_target(input, to_surface, u, v);

    float flash = 0.0f;
    if (t < 0.5f) {
        flash = t * 2.0f;
    } else {
        flash = (1.0f - t) * 2.0f;
    }

    const Color base = Color::lerp(a, b, t);
    const Color flash_color = {1.0f, 1.0f, 1.0f, 1.0f};
    return Color::lerp(base, flash_color, flash * 0.8f);
}

namespace {



Color transition_ripple(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float dist = std::sqrt(dx * dx + dy * dy);
    
    const float wave = std::sin(dist * 30.0f - t * 20.0f) * 0.03f * (1.0f - t);
    const float ru = u + wave * (dx / dist);
    const float rv = v + wave * (dy / dist);
    
    const Color a = sample_uv(input, ru, rv);
    const Color b = sample_transition_target(input, to_surface, u, v);
    return Color::lerp(a, b, t);
}

} // namespace

struct ArtisticTransitionRegistryEntry {
    std::string_view id;
    CpuTransitionFn cpu_fn;
};

static constexpr std::array<ArtisticTransitionRegistryEntry, 6> kArtisticTransitionsRegistry = {{
    { ids::transition::zoom_in, transition_zoom_in },
    { ids::transition::zoom_blur, transition_zoom_blur },
    { ids::transition::spin, transition_spin },
    { ids::transition::directional_blur_wipe, transition_directional_blur_wipe },
    { ids::transition::flash, transition_flash },
    { ids::transition::ripple, transition_ripple }
}};

consteval bool has_artistic_registry_duplicates() {
    for (size_t i = 0; i < kArtisticTransitionsRegistry.size(); ++i) {
        for (size_t j = i + 1; j < kArtisticTransitionsRegistry.size(); ++j) {
            if (kArtisticTransitionsRegistry[i].id == kArtisticTransitionsRegistry[j].id) return true;
        }
    }
    return false;
}

static_assert(!has_artistic_registry_duplicates(), "FATAL: Duplicate transition identifier detected within the Artistic Transitions Registry.");

void resolve_artistic_transition_implementations(tachyon::TransitionDescriptor& d) {
    for (const auto& entry : kArtisticTransitionsRegistry) {
        if (entry.id == d.id) {
            d.cpu_fn = entry.cpu_fn;
            return;
        }
    }
}

} // namespace tachyon::renderer2d
