#include "tachyon/renderer3d/temporal/frame_blend_mode.h"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace tachyon::renderer3d::temporal {

float FrameBlender::blend_factor(int32_t frame_offset) const {
  switch (params_.mode) {
    case FrameBlendMode::kHold:
      return frame_offset == 0 ? 1.0f : 0.0f;
    case FrameBlendMode::kLinear: {
      float abs_offset = std::abs(static_cast<float>(frame_offset));
      return abs_offset < params_.blend_window ? 1.0f - (abs_offset / params_.blend_window) : 0.0f;
    }
    case FrameBlendMode::kOpticalFlow:
      return 1.0f;
    default:
      throw std::invalid_argument("Unknown blend mode");
  }
}

std::string FrameBlender::cache_identity() const {
  std::ostringstream oss;
  oss << "blend_mode:" << static_cast<int>(params_.mode)
      << "|blend_window:" << params_.blend_window;
  return oss.str();
}

} // namespace
