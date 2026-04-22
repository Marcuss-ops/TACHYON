#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"

#include <cstdint>
#include <vector>
#include <optional>

namespace tachyon::renderer2d {

enum class PathVerb {
    MoveTo,
    LineTo,
    CubicTo,
    Close
};

enum class WindingRule {
    NonZero,
    EvenOdd
};

struct PathCommand {
    PathVerb verb{PathVerb::MoveTo};
    math::Vector2 p0{};
    math::Vector2 p1{};
    math::Vector2 p2{};
};

struct PathGeometry {
    std::vector<PathCommand> commands;
};

struct FillPathStyle {
    Color fill_color{Color::white()};
    std::optional<GradientSpec> gradient;
    float opacity{1.0f};
    WindingRule winding{WindingRule::NonZero};
};

enum class LineCap {
    Butt,
    Round,
    Square
};

enum class LineJoin {
    Miter,
    Round,
    Bevel
};

struct StrokePathStyle {
    Color stroke_color{Color::white()};
    std::optional<GradientSpec> gradient;
    float stroke_width{1.0f};
    float opacity{1.0f};
    LineCap cap{LineCap::Butt};
    LineJoin join{LineJoin::Miter};
    float miter_limit{4.0f};
};

struct GradientLUT {
    static constexpr int SAMPLES = 256;
    float r[SAMPLES];
    float g[SAMPLES];
    float b[SAMPLES];
    float a[SAMPLES];

    GradientLUT(const GradientSpec& spec) {
        if (spec.stops.empty()) {
            std::fill(r, r + SAMPLES, 1.0f);
            std::fill(g, g + SAMPLES, 1.0f);
            std::fill(b, b + SAMPLES, 1.0f);
            std::fill(a, a + SAMPLES, 1.0f);
            return;
        }

        for (int i = 0; i < SAMPLES; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(SAMPLES - 1);
            
            auto it = std::lower_bound(spec.stops.begin(), spec.stops.end(), t, [](const GradientStop& s, float val) {
                return s.offset < val;
            });

            Color c;
            if (it == spec.stops.begin()) {
                c = Color{it->color.r / 255.0f, it->color.g / 255.0f, it->color.b / 255.0f, it->color.a / 255.0f};
            } else if (it == spec.stops.end()) {
                const auto& last = spec.stops.back();
                c = Color{last.color.r / 255.0f, last.color.g / 255.0f, last.color.b / 255.0f, last.color.a / 255.0f};
            } else {
                const auto& s1 = *(it - 1);
                const auto& s2 = *it;
                float range = s2.offset - s1.offset;
                float alpha = (range > 1e-6f) ? (t - s1.offset) / range : 0.0f;
                c = Color{
                    (s1.color.r * (1.0f - alpha) + s2.color.r * alpha) / 255.0f,
                    (s1.color.g * (1.0f - alpha) + s2.color.g * alpha) / 255.0f,
                    (s1.color.b * (1.0f - alpha) + s2.color.b * alpha) / 255.0f,
                    (s1.color.a * (1.0f - alpha) + s2.color.a * alpha) / 255.0f
                };
            }
            r[i] = c.r; g[i] = c.g; b[i] = c.b; a[i] = c.a;
        }
    }

    inline Color sample(float t) const {
        int idx = std::clamp(static_cast<int>(t * (SAMPLES - 1)), 0, SAMPLES - 1);
        return {r[idx], g[idx], b[idx], a[idx]};
    }
};

} // namespace tachyon::renderer2d
