#pragma once

#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include "tachyon/core/spec/schema/objects/path_spec.h"

namespace tachyon {
namespace spec {

PathGeometry build_geometry(const ShapeSpec& spec);

} // namespace spec
} // namespace tachyon