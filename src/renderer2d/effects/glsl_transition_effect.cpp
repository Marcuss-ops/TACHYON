#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace tachyon::renderer2d {
namespace {

Color sample_uv(const SurfaceRGBA& surface, float u, float v) {
    return sample_texture_bilinear(surface, std::clamp(u, 0.0f, 1.0f), std::clamp(v, 0.0f, 1.0f), Color::white());
}

Color sample_transition_source(const SurfaceRGBA& input, const SurfaceRGBA* aux, float u, float v) {
    if (aux != nullptr) {
        return sample_uv(*aux, u, v);
    }
    return sample_uv(input, u, v);
}

std::string transition_id_from_params(const EffectParams& params) {
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
    return clamp01(get_scalar(params, "t", get_scalar(params, "progress", 0.0f)));
}

Color lerp_surface_color(const SurfaceRGBA& a, const SurfaceRGBA* b, float u, float v, float t) {
    const Color ca = sample_uv(a, u, v);
    const Color cb = sample_transition_source(a, b, u, v);
    return Color::lerp(ca, cb, clamp01(t));
}

}  // namespace

SurfaceRGBA GlslTransitionEffect::apply(const SurfaceRGBA& input, const EffectParams& params) const {
    const float t = transition_progress(params);
    const auto transition = transition_id_from_params(params);

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
    const float angle = t * 6.28318530718f;
    const float eased = t * t * (3.0f - 2.0f * t);
    const float zoom = std::max(0.001f, 1.0f - t);

    for (std::uint32_t y = 0; y < input.height(); ++y) {
        for (std::uint32_t x = 0; x < input.width(); ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / width;
            const float v = (static_cast<float>(y) + 0.5f) / height;
            Color out = sample_transition_source(input, to_surface, u, v);

            if (transition == "fade_to_black") {
                const Color black = Color::black();
                if (t < 0.5f) {
                    out = Color::lerp(sample_uv(input, u, v), black, t * 2.0f);
                } else {
                    const float tt = (t - 0.5f) * 2.0f;
                    out = Color::lerp(black, sample_transition_source(input, to_surface, u, v), tt);
                }
            } else if (transition == "wipe_linear") {
                out = (u < t) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
            } else if (transition == "wipe_angular") {
                const float ux = u - 0.5f;
                const float vy = v - 0.5f;
                const float a = std::atan2(vy, ux) + 3.14159265359f;
                out = (a < angle) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
            } else if (transition == "push_left") {
                out = sample_transition_source(input, to_surface, u + t, v);
            } else if (transition == "slide_easing") {
                out = sample_transition_source(input, to_surface, u + eased, v);
            } else if (transition == "zoom_in") {
                const float su = 0.5f + (u - 0.5f) / zoom;
                const float sv = 0.5f + (v - 0.5f) / zoom;
                out = sample_transition_source(input, to_surface, su, sv);
            } else if (transition == "zoom_blur") {
                const float su = 0.5f + (u - 0.5f) / zoom;
                const float sv = 0.5f + (v - 0.5f) / zoom;
                Color acc = Color::transparent();
                constexpr int samples = 6;
                for (int i = 0; i < samples; ++i) {
                    const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
                    const float offset = (s - 0.5f) * t * 0.12f;
                    acc = Color::lerp(acc, sample_transition_source(input, to_surface, su + offset, sv + offset), 1.0f / static_cast<float>(samples));
                }
                out = acc;
            } else if (transition == "spin") {
                const float dx = u - 0.5f;
                const float dy = v - 0.5f;
                const float c = std::cos(angle);
                const float s = std::sin(angle);
                const float ru = 0.5f + dx * c - dy * s;
                const float rv = 0.5f + dx * s + dy * c;
                out = sample_transition_source(input, to_surface, ru, rv);
            } else if (transition == "circle_iris") {
                const float radius = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
                out = (radius < t * 0.75f) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
            } else if (transition == "pixelate") {
                const float grid = std::max(2.0f, 20.0f - 19.0f * t);
                const float pu = std::floor(u * grid) / grid;
                const float pv = std::floor(v * grid) / grid;
                out = sample_transition_source(input, to_surface, pu, pv);
            } else if (transition == "glitch_slice") {
                const float band = std::floor(v * 10.0f);
                const float jitter = std::sin(band * 12.0f + t * 10.0f) * 0.02f;
                out = sample_transition_source(input, to_surface, u + jitter, v);
            } else if (transition == "rgb_split") {
                const float r = sample_transition_source(input, to_surface, u + 0.01f * t, v).r;
                const float g = sample_transition_source(input, to_surface, u, v).g;
                const float b = sample_transition_source(input, to_surface, u - 0.01f * t, v).b;
                const float a = sample_transition_source(input, to_surface, u, v).a;
                out = {r, g, b, a};
            } else if (transition == "luma_dissolve") {
                const float noise = std::fmod(std::sin((u * 123.4f + v * 456.7f) * 12.9898f) * 43758.5453f, 1.0f);
                out = (noise < t) ? sample_transition_source(input, to_surface, u, v) : sample_uv(input, u, v);
            } else if (transition == "directional_blur_wipe") {
                const float slide = std::clamp(t * 1.5f, 0.0f, 1.0f);
                const float sigma = 0.01f + 0.04f * t;
                Color acc = Color::transparent();
                constexpr int samples = 5;
                for (int i = 0; i < samples; ++i) {
                    const float s = static_cast<float>(i) / static_cast<float>(samples - 1);
                    const float off = (s - 0.5f) * sigma;
                    acc = Color::lerp(acc, sample_transition_source(input, to_surface, u + slide + off, v), 1.0f / static_cast<float>(samples));
                }
                out = acc;
            } else {
                out = lerp_surface_color(input, to_surface, u, v, t);
            }

            output.set_pixel(x, y, out);
        }
    }

    return output;
}

}  // namespace tachyon::renderer2d
