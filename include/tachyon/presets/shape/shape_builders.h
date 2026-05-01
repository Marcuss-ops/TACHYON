#pragma once

#include "tachyon/presets/shape/shape_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::presets {

[[nodiscard]] LayerSpec build_shape(const ShapeParams& p);          // dispatcher su p.type

[[nodiscard]] LayerSpec build_shape_rect(const ShapeParams& p);
[[nodiscard]] LayerSpec build_shape_circle(const ShapeParams& p);
[[nodiscard]] LayerSpec build_shape_ellipse(const ShapeParams& p);
[[nodiscard]] LayerSpec build_shape_line(const ShapeParams& p);

} // namespace tachyon::presets
