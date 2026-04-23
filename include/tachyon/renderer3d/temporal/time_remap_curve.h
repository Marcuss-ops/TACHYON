#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <optional>

namespace tachyon::renderer3d::temporal {

enum class TimeRemapMode { kHold, kBlend, kOpticalFlow };

struct TimeRemapParams {
  TimeRemapMode mode;
  float shutter_angle;
  float shutter_phase;
  uint32_t sample_count;
  std::function<float(float)> fallback_policy;
};

class TimeRemapCurve {
public:
  explicit TimeRemapCurve(TimeRemapParams params) : params_(std::move(params)) {}

  float evaluate(float target_time, float frame_duration, std::optional<float> flow_confidence = std::nullopt) const;
  std::string cache_identity() const;

private:
  TimeRemapParams params_;
};

} // namespace
