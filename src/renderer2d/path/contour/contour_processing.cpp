#include "tachyon/renderer2d/path/contour/contour_processing.h"
#include "tachyon/renderer2d/path/flattening/path_flattening.h"
#include <cmath>

namespace tachyon::renderer2d {

std::vector<Contour> ContourProcessor::build_contours(const PathGeometry& path, float tolerance) {
    (void)tolerance;
    return ::tachyon::renderer2d::build_contours(path);
}

} // namespace tachyon::renderer2d
