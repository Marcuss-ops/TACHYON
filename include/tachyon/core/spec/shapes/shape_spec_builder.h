#pragma once

#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include "tachyon/renderer2d/raster/path/path_types.h"

namespace tachyon {
namespace spec {

renderer2d::PathGeometry build_geometry(const ShapeSpec& spec);

} // namespace spec
} // namespace tachyon