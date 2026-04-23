#pragma once
#include <string>
#include <cstdint>

namespace tachyon::renderer3d::temporal {

enum class FrameBlendMode { kHold, kLinear, kOpticalFlow };

struct FrameBlendParams {
  FrameBlendMode mode;
  uint32_t blend_window;
};

class FrameBlender {
public:
  explicit FrameBlender(FrameBlendParams params) : params_(std::move(params)) {}

  float blend_factor(int32_t frame_offset) const;
  std::string cache_identity() const;

private:
  FrameBlendParams params_;
};

} // namespace
