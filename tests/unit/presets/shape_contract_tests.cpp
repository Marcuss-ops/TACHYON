#include "tachyon/presets/shape/shape_builders.h"
#include "tachyon/presets/shape/shape_params.h"
#include <cassert>

int main() {
    tachyon::presets::ShapeParams p;
    p.type = "rect";
    p.fill_color = "#ff0000";
    p.stroke_color = "#000000";
    p.stroke_width = 2.0f;
    p.corner_radius = 4.0f;
    p.in_point = 0.0;
    p.out_point = 3.0;
    p.x = 100.0f;
    p.y = 100.0f;
    p.w = 200.0f;
    p.h = 150.0f;

    auto spec = tachyon::presets::build_shape(p);
    assert(spec.type == "shape");
    assert(spec.shape.type == "rect");
    assert(spec.shape.fill_color == "#ff0000");
    assert(spec.in_point == 0.0);
    assert(spec.out_point == 3.0);

    // Test circle variant
    p.type = "circle";
    auto circle_spec = tachyon::presets::build_shape_circle(p);
    assert(circle_spec.shape.type == "circle");

    return 0;
}
