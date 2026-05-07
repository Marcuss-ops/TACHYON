#include "tachyon/core/spec/shapes/shape_spec_builder.h"
#include "tachyon/core/spec/shapes/shape_factory.h"

namespace tachyon {
namespace spec {

PathGeometry build_geometry(const ShapeSpec& spec) {
    if (spec.type == "rectangle")
        return ShapeFactory::create_rectangle(spec.x, spec.y, spec.width, spec.height);

    if (spec.type == "rounded_rect")
        return ShapeFactory::create_rounded_rectangle(spec.x, spec.y, spec.width, spec.height, spec.radius);

    if (spec.type == "circle")
        return ShapeFactory::create_circle(spec.x, spec.y, spec.radius);

    if (spec.type == "ellipse")
        return ShapeFactory::create_ellipse(spec.x, spec.y, spec.width * 0.5f, spec.height * 0.5f);

    if (spec.type == "line")
        return ShapeFactory::create_line(spec.x, spec.y, spec.x1, spec.y1);

    if (spec.type == "arrow")
        return ShapeFactory::create_arrow(spec.x, spec.y, spec.x1, spec.y1, spec.head_size);

    if (spec.type == "polygon")
        return ShapeFactory::create_polygon(spec.x, spec.y, spec.sides, spec.radius);

    if (spec.type == "star")
        return ShapeFactory::create_star(spec.x, spec.y, spec.sides, spec.inner_radius, spec.outer_radius);

    if (spec.type == "speech_bubble")
        return ShapeFactory::create_speech_bubble(spec.x, spec.y, spec.width, spec.height, spec.radius, spec.tail_x, spec.tail_y);

    if (spec.type == "callout")
        return ShapeFactory::create_callout(spec.x, spec.y, spec.width, spec.height, spec.tail_x, spec.tail_y);

    if (spec.type == "badge")
        return ShapeFactory::create_badge(spec.x, spec.y, spec.radius, spec.sides);

    return {};
}

} // namespace spec
} // namespace tachyon
