#pragma once

#include "tachyon/renderer2d/raster/path/path_types.h"

namespace tachyon::renderer2d {

PathGeometry trim_path(const PathGeometry& path, float start, float end, float offset);

} // namespace tachyon::renderer2d
