#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"

#include <cstdint>
#include <vector>

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

class PathRasterizer {
public:
    static void fill(SurfaceRGBA& surface, const PathGeometry& path, const FillPathStyle& style);
    static void stroke(SurfaceRGBA& surface, const PathGeometry& path, const StrokePathStyle& style);

    // Trims the path to the range [start, end] (0.0 to 1.0), with an optional offset (0.0 to 1.0).
    // AE-compatible trim path behavior.
    static PathGeometry trim(const PathGeometry& path, float start, float end, float offset);
};

} // namespace tachyon::renderer2d
