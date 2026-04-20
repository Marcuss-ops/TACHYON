#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/gradient_spec.h"

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
};

} // namespace tachyon::renderer2d
