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

} // namespace tachyon::renderer2d
