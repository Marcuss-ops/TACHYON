#include "tachyon/renderer3d/temporal/time_remap_curve.h"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace tachyon::renderer3d::temporal {

float TimeRemapCurve::evaluate(float target_time, float frame_duration, std::optional<float> flow_confidence) const {
  switch (params_.mode) {
    case TimeRemapMode::kHold:
      return std::floor(target_time / frame_duration) * frame_duration;
    case TimeRemapMode::kBlend:
      return target_time;
    case TimeRemapMode::kOpticalFlow: {
      if (!flow_confidence.has_value()) {
        throw std::invalid_argument("Optical flow mode requires confidence value");
      }
      const float conf = flow_confidence.value();
      if (conf < 0.5f && params_.fallback_policy) {
        return params_.fallback_policy(conf);
      }
      return target_time;
    }
    default:
      throw std::invalid_argument("Unknown time remap mode");
  }
}

std::string TimeRemapCurve::cache_identity() const {
  std::ostringstream oss;
  oss << "remap_mode:" << static_cast<int>(params_.mode)
      << "|shutter_angle:" << params_.shutter_angle
      << "|shutter_phase:" << params_.shutter_phase
      << "|samples:" << params_.sample_count;
  return oss.str();
}

} // namespace
