#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/core/effect_utils.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/animation/easing.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>

namespace tachyon::renderer2d {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

namespace {  // Local helpers

Color sample_uv(const SurfaceRGBA& surface, float u, float v) {
    return sample_texture_bilinear(surface, std::clamp(u, 0.0f, 1.0f), std::clamp(v, 0.0f, 1.0f), Color::white());
}

Color sample_transition_source(const SurfaceRGBA& input, const SurfaceRGBA* aux, float u, float v) {
    if (aux != nullptr) {
        return sample_uv(*aux, u, v);
    }
    return sample_uv(input, u, v);
}

[[maybe_unused]] std::string transition_id_from_params(const EffectParams& params) {
    if (const auto transition_it = params.strings.find("transition_id"); transition_it != params.strings.end()) {
        return transition_it->second;
    }
    if (const auto shader_it = params.strings.find("shader_path"); shader_it != params.strings.end()) {
        return std::filesystem::path(shader_it->second).stem().string();
    }
    if (const auto shader_name_it = params.strings.find("shader"); shader_name_it != params.strings.end()) {
        return std::filesystem::path(shader_name_it->second).stem().string();
    }
    return "crossfade";
}

float transition_progress(const EffectParams& params) {
    float raw_t = clamp01(get_scalar(params, "t", get_scalar(params, "progress", 0.0f)));
    
    const auto preset_it = params.scalars.find("easing_preset");
    if (preset_it == params.scalars.end()) {
        return raw_t;
    }
    
    const int preset_val = static_cast<int>(preset_it->second);
    if (preset_val < 0 || preset_val > static_cast<int>(animation::EasingPreset::Custom)) {
        return raw_t;
    }
    
    const animation::EasingPreset preset = static_cast<animation::EasingPreset>(preset_val);
    animation::CubicBezierEasing bezier;
    
    if (preset == animation::EasingPreset::Custom) {
        bezier.cx1 = get_scalar(params, "bezier_cx1", 0.0f);
        bezier.cy1 = get_scalar(params, "bezier_cy1", 0.0f);
        bezier.cx2 = get_scalar(params, "bezier_cx2", 1.0f);
        bezier.cy2 = get_scalar(params, "bezier_cy2", 1.0f);
    }
    
    const double eased_t = animation::apply_easing(static_cast<double>(raw_t), preset, bezier);
    return static_cast<float>(clamp01(eased_t));
}

Color lerp_surface_color(const SurfaceRGBA& a, const SurfaceRGBA* b, float u, float v, float t) {
    const Color ca = sample_uv(a, u, v);
    const Color cb = sample_transition_source(a, b, u, v);
    return Color::lerp(ca, cb, clamp01(t));
}

Color screen_over(const Color& base, const Color& overlay, float intensity) {
    const float alpha = clamp01(intensity);
    return {
        1.0f - (1.0f - base.r) * (1.0f - overlay.r * alpha),
        1.0f - (1.0f - base.g) * (1.0f - overlay.g * alpha),
        1.0f - (1.0f - base.b) * (1.0f - overlay.b * alpha),
        1.0f
    };
}

// Transition function implementations

// Base transitions (pixel-level versions)
Color transition_fade(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_source(input, to_surface, u, v);
    return Color::lerp(a, b, t);
}

Color transition_slide(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    // Slide from input (right) to to_surface (left)
    return sample_transition_source(input, to_surface, u - t, v);
}

Color transition_zoom(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = 1.0f + t;  // Zoom from 1.0 to 2.0
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    return Color::lerp(sample_uv(input, u, v), sample_transition_source(input, to_surface, su, sv), t);
}

Color transition_flip(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    // Simple horizontal flip effect
    const float fu = (u < 0.5f) ? u * 2.0f : (1.0f - u) * 2.0f;
    return Color::lerp(sample_uv(input, fu, v), sample_transition_source(input, to_surface, u, v), t);
}

Color transition_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    // Simple blur transition (box blur approximation)
    const float radius = t * 0.1f;
    Color acc = Color::transparent();
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const float offset = (static_cast<float>(i) / static_cast<float>(samples - 1) - 0.5f) * radius;
        acc = Color::lerp(acc, sample_uv(input, u + offset, v), 1.0f / static_cast<float>(samples));
    }
    return Color::lerp(acc, sample_transition_source(input, to_surface, u, v), t);
}

Color transition_fade_to_black(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color black = Color::black();
    if (t < 0.5f) {
        return Color::lerp(sample_uv(input, u, v), black, t * 2.0f);
    } else {
        const float tt = (t - 0.5f) * 2.0f;
        return Color::lerp(black, sample_transition_source(input, to_surface, u, v), tt);
    }
}

Color transition_wipe_linear(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return (u < t) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_wipe_angular(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float angle = t * 6.28318530718f;
    const float ux = u - 0.5f;
    const float vy = v - 0.5f;
    const float a = std::atan2(vy, ux) + 3.14159265359f;
    return (a < angle) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_push_left(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    return sample_transition_source(input, to_surface, u + t, v);
}

Color transition_slide_easing(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float eased = t * t * (3.0f - 2.0f * t);
    return sample_transition_source(input, to_surface, u + eased, v);
}

Color transition_zoom_in(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    return sample_transition_source(input, to_surface, su, sv);
}

Color transition_zoom_blur(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float zoom = std::max(0.001f, 1.0f - t);
    const float su = 0.5f + (u - 0.5f) / zoom;
    const float sv = 0.5f + (v - 0.5f) / zoom;
    Color acc = Color::transparent();
    constexpr int samples = 6;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float offset = (s - 0.5f) * t * 0.12f;
        acc = Color::lerp(acc, sample_transition_source(input, to_surface, su + offset, sv + offset), 1.0f / static_cast<float>(samples));
    }
    return acc;
}

Color transition_spin(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float angle = t * 6.28318530718f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float ru = 0.5f + dx * c - dy * s;
    const float rv = 0.5f + dx * s + dy * c;
    return sample_transition_source(input, to_surface, ru, rv);
}

Color transition_circle_iris(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float radius = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
    return (radius < t * 0.75f) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_pixelate(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float grid = std::max(2.0f, 20.0f - 19.0f * t);
    const float pu = std::floor(u * grid) / grid;
    const float pv = std::floor(v * grid) / grid;
    return sample_transition_source(input, to_surface, pu, pv);
}

Color transition_glitch_slice(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float band = std::floor(v * 10.0f);
    const float jitter = std::sin(band * 12.0f + t * 10.0f) * 0.02f;
    return sample_transition_source(input, to_surface, u + jitter, v);
}

Color transition_rgb_split(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float r = sample_transition_source(input, to_surface, u + 0.01f * t, v).r;
    const float g = sample_transition_source(input, to_surface, u, v).g;
    const float b = sample_transition_source(input, to_surface, u - 0.01f * t, v).b;
    const float a = sample_transition_source(input, to_surface, u, v).a;
    return {r, g, b, a};
}

Color transition_luma_dissolve(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float noise = std::fmod(std::sin((u * 123.4f + v * 456.7f) * 12.9898f) * 43758.5453f, 1.0f);
    return (noise < t) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
}

Color transition_directional_blur_wipe(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const float slide = std::clamp(t * 1.5f, 0.0f, 1.0f);
    const float sigma = 0.01f + 0.04f * t;
    Color acc = Color::transparent();
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
        const float off = (s - 0.5f) * sigma;
        acc = Color::lerp(acc, sample_transition_source(input, to_surface, u + slide + off, v), 1.0f / static_cast<float>(samples));
    }
    return acc;
}

Color transition_flash(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const Color a = sample_uv(input, u, v);
    const Color b = sample_transition_source(input, to_surface, u, v);
    
    // Flash intensity peaks at t=0.5
    float flash = 0.0f;
    if (t < 0.5f) {
        flash = t * 2.0f; // fade in flash
    } else {
        flash = (1.0f - t) * 2.0f; // fade out flash
    }
    
    Color base = Color::lerp(a, b, t);
    Color flash_color = {1.0f, 1.0f, 1.0f, 1.0f};
    
    return Color::lerp(base, flash_color, flash * 0.8f);
}

Color transition_light_leak(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_source(input, to_surface, u, v), t);

    // Diagonal amber sweep. This is intentionally a band, not a radial flash.
    const float angle = -22.6f * 3.14159f / 180.0f;
    const float ca = std::cos(angle);
    const float sa = std::sin(angle);
    float proj = u * ca + v * sa;
    proj = (proj + 0.2f) / 1.4f;

    float pos = -0.3f + t * 1.6f;
    pos += 0.02f * std::sin(t * 6.28318f);

    const float width = 0.40f;
    const float d = proj - pos;
    const float mask = std::exp(-(d * d) / (2.0f * width * width));
    // Wide ambient layer keeps the glow visible even when band center is off-screen.
    const float ambient = std::exp(-(d * d) / (2.0f * 0.70f * 0.70f)) * 0.30f;

    const Color ca_col = {1.0f, 0.55f, 0.08f, 1.0f};
    const Color cb_col = {1.0f, 0.765f, 0.275f, 1.0f};
    const Color leak = Color::lerp(ca_col, cb_col, std::clamp(proj, 0.0f, 1.0f));
    const float intensity = 1.1f * (mask * 0.80f + ambient);
    return screen_over(base, leak, intensity);
}

Color transition_film_burn(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface) {
    const bool overlay_mode = (to_surface == nullptr);
    const Color base = overlay_mode ? Color::black() : Color::lerp(sample_uv(input, u, v), sample_transition_source(input, to_surface, u, v), t);

    // Fiery burn sweep, also linear and screen-blended.
    const float angle = -22.6f * 3.14159f / 180.0f;
    const float ca = std::cos(angle);
    const float sa = std::sin(angle);
    const float proj = (u * ca + v * sa + 0.2f) / 1.4f;

    const float pos = -0.3f + t * 1.6f;
    const float width = 0.32f;
    const float d = proj - pos;
    float intensity = std::exp(-(d * d) / (2.0f * width * width));

    const Color ca_col = {1.0f, 0.32f, 0.0f, 1.0f};
    const Color cb_col = {1.0f, 0.667f, 0.137f, 1.0f};
    const Color burn = Color::lerp(ca_col, cb_col, std::clamp(proj, 0.0f, 1.0f));

    const float jitter = std::fmod(std::sin((u * 123.4f + v * 456.7f) * 12.9898f) * 43758.5453f, 0.15f);
    intensity = std::clamp(1.05f * intensity + jitter, 0.0f, 1.2f);
    return screen_over(base, burn, intensity);
}

}  // namespace

void init_builtin_transitions() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        auto& reg = TransitionRegistry::instance();

        // Base transitions (pixel-level versions)
        reg.register_transition({"fade", "Fade", "Simple crossfade transition", transition_fade});
        reg.register_transition({"slide", "Slide", "Slide transition", transition_slide});
        reg.register_transition({"zoom", "Zoom", "Zoom transition", transition_zoom});
        reg.register_transition({"flip", "Flip", "Flip transition", transition_flip});
        reg.register_transition({"blur", "Blur", "Blur transition", transition_blur});

        // Advanced transitions
        reg.register_transition({"fade_to_black", "Fade to Black", "Crossfade through black", transition_fade_to_black});
        reg.register_transition({"wipe_linear", "Linear Wipe", "Simple left-to-right wipe", transition_wipe_linear});
        reg.register_transition({"wipe_angular", "Angular Wipe", "Angular wipe around center", transition_wipe_angular});
        reg.register_transition({"push_left", "Push Left", "Push image to the left", transition_push_left});
        reg.register_transition({"slide_easing", "Slide Easing", "Slide with easing", transition_slide_easing});
        reg.register_transition({"zoom_in", "Zoom In", "Zoom into target", transition_zoom_in});
        reg.register_transition({"zoom_blur", "Zoom Blur", "Zoom with motion blur", transition_zoom_blur});
        reg.register_transition({"spin", "Spin", "Spin rotation", transition_spin});
        reg.register_transition({"circle_iris", "Circle Iris", "Circular iris opener", transition_circle_iris});
        reg.register_transition({"pixelate", "Pixelate", "Pixelation transition", transition_pixelate});
        reg.register_transition({"glitch_slice", "Glitch Slice", "Glitchy slice effect", transition_glitch_slice});
        reg.register_transition({"rgb_split", "RGB Split", "Color channel split", transition_rgb_split});
        reg.register_transition({"luma_dissolve", "Luma Dissolve", "Luminance-based dissolve", transition_luma_dissolve});
        reg.register_transition({"directional_blur_wipe", "Directional Blur Wipe", "Blur wipe with direction", transition_directional_blur_wipe});
        reg.register_transition({"flash", "Flash", "White flash transition", transition_flash});

        // Cinematic transitions
        reg.register_transition({"light_leak", "Light Leak", "Warm orange light leak", transition_light_leak});
        reg.register_transition({"film_burn", "Film Burn", "Fiery red-orange film burn", transition_film_burn});
    });
}

SurfaceRGBA GlslTransitionEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    init_builtin_transitions();
    
    const float t = transition_progress(params);
    
    const TransitionSpec* transition_spec = nullptr;
    const auto transition_it = params.strings.find("transition_id");
    if (transition_it != params.strings.end()) {
        transition_spec = TransitionRegistry::instance().find(transition_it->second);
    }
    
    const SurfaceRGBA* to_surface = nullptr;
    if (const auto to_it = params.aux_surfaces.find("transition_to"); to_it != params.aux_surfaces.end()) {
        to_surface = to_it->second;
    } else if (const auto to_fallback_it = params.aux_surfaces.find("to"); to_fallback_it != params.aux_surfaces.end()) {
        to_surface = to_fallback_it->second;
    } else if (const auto bg_it = params.aux_surfaces.find("background"); bg_it != params.aux_surfaces.end()) {
        to_surface = bg_it->second;
    }
    
    SurfaceRGBA output(input.width(), input.height());
    output.set_profile(input.profile());
    if (input.width() == 0U || input.height() == 0U) {
        return output;
    }
    
    const float width = static_cast<float>(std::max<std::uint32_t>(1U, input.width()));
    const float height = static_cast<float>(std::max<std::uint32_t>(1U, input.height()));
    
    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / width;
            const float v = (static_cast<float>(y) + 0.5f) / height;
            
            Color out;
            if (transition_spec != nullptr && transition_spec->function != nullptr) {
                out = transition_spec->function(u, v, t, input, to_surface);
            } else {
                out = lerp_surface_color(input, to_surface, u, v, t);
            }
            
            output.set_pixel(x, y, out);
        }
    }
    
    return output;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace tachyon::renderer2d
