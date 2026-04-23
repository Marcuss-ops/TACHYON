#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::renderer3d::temporal {

enum class RollingShutterMode { kNone, kTopToBottom, kBottomToTop, kLeftToRight, kRightToLeft };

struct RollingShutterConfig {
    RollingShutterMode mode{RollingShutterMode::kNone};
    float scan_time{0.0f}; // time for full scan in seconds
    float shutter_angle{180.0f};
};

class RollingShutterEffect {
public:
    explicit RollingShutterEffect(RollingShutterConfig config = {});

    void set_config(const RollingShutterConfig& config);
    const RollingShutterConfig& get_config() const { return config_; }

    bool is_enabled() const { return config_.mode != RollingShutterMode::kNone && config_.scan_time > 0.0f; }

    // Returns the effective exposure time for a given scanline/sample position (0.0 to 1.0)
    float exposure_offset(float position) const;

    // Adjust subframe time based on rolling shutter
    std::vector<double> adjust_subframe_times(const std::vector<double>& times, float frame_height) const;

    std::string cache_identity() const;

private:
    RollingShutterConfig config_;
};

} // namespace
