#pragma once

#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/spec/schema/common/gradient_spec.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include "tachyon/renderer2d/raster/path/path_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <vector>
#include <string>

namespace tachyon {
namespace media {

struct ParsedSvg {
  std::vector<shapes::ShapePathSpec> shapes;
  std::vector<GradientSpec> gradients;
  std::vector<renderer2d::MaskPath> masks;
  std::vector<renderer2d::FillPathStyle> fill_styles;
  std::vector<renderer2d::StrokePathStyle> stroke_styles;
};

bool parse_svg_string(const std::string& svg_content, ParsedSvg& out_result, DiagnosticBag& diagnostics);

} // namespace media
} // namespace tachyon
