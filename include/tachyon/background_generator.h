#pragma once
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>

namespace tachyon {

struct ShapeGridParams {
    std::string shape = "square";       // "square" or "circle"
    float spacing = 40.0f;              // Size of each grid cell
    float border_width = 1.0f;          // Width of grid borders
    ColorSpec border_color = {150, 150, 150, 255}; // Grid border color
    ColorSpec background_color = {0, 0, 0, 255}; // color_a (background)
    ColorSpec grid_color = {150, 150, 150, 255}; // color_b (grid lines)
    float speed = 1.0f;                 // Animation speed
    std::string direction = "right";    // "right", "left", "up", "down", "diagonal"
    uint64_t seed = 0;                  // Random seed
};

// Generates a reusable ShapeGrid ProceduralSpec for background rendering
ProceduralSpec GenerateShapeGridBackground(const ShapeGridParams& params);

} // namespace tachyon
