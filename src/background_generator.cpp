#include "tachyon/background_generator.h"
#include <cmath>

namespace tachyon {

ProceduralSpec GenerateShapeGridBackground(const ShapeGridParams& params) {
    ProceduralSpec spec;
    spec.kind = "tachyon.background.kind.grid";
    spec.shape = params.shape;
    spec.spacing = AnimatedScalarSpec(params.spacing);
    spec.border_width = AnimatedScalarSpec(params.border_width);
    spec.border_color = AnimatedColorSpec(params.border_color);
    spec.color_a = AnimatedColorSpec(params.background_color);
    spec.color_b = AnimatedColorSpec(params.grid_color);
    spec.speed = AnimatedScalarSpec(params.speed);
    spec.seed = params.seed;

    // Map direction string to angle (degrees) for movement
    float angle_deg = 0.0f;
    if (params.direction == "left") {
        angle_deg = 180.0f;
    } else if (params.direction == "up") {
        angle_deg = 90.0f;
    } else if (params.direction == "down") {
        angle_deg = 270.0f;
    } else if (params.direction == "diagonal") {
        angle_deg = 45.0f;
    } else { // "right" or default
        angle_deg = 0.0f;
    }
    spec.angle = AnimatedScalarSpec(angle_deg);

    return spec;
}

} // namespace tachyon
