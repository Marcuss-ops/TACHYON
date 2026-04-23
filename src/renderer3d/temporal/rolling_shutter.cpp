#include "tachyon/renderer3d/temporal/rolling_shutter.h"
#include <sstream>
#include <cmath>

namespace tachyon::renderer3d::temporal {

RollingShutterEffect::RollingShutterEffect(RollingShutterConfig config) : config_(std::move(config)) {}

void RollingShutterEffect::set_config(const RollingShutterConfig& config) {
    config_ = config;
}

float RollingShutterEffect::exposure_offset(float position) const {
    if (!is_enabled()) return 0.0f;
    switch (config_.mode) {
        case RollingShutterMode::kTopToBottom:
            return position * config_.scan_time;
        case RollingShutterMode::kBottomToTop:
            return (1.0f - position) * config_.scan_time;
        case RollingShutterMode::kLeftToRight:
            return position * config_.scan_time;
        case RollingShutterMode::kRightToLeft:
            return (1.0f - position) * config_.scan_time;
        default:
            return 0.0f;
    }
}

std::vector<double> RollingShutterEffect::adjust_subframe_times(const std::vector<double>& times, float frame_height) const {
    if (!is_enabled() || times.empty()) return times;
    std::vector<double> adjusted;
    adjusted.reserve(times.size());
    for (size_t i = 0; i < times.size(); ++i) {
        // For simplicity, assume each subframe corresponds to a scanline position
        float position = static_cast<float>(i) / static_cast<float>(times.size() - 1);
        adjusted.push_back(times[i] + exposure_offset(position));
    }
    return adjusted;
}

std::string RollingShutterEffect::cache_identity() const {
    std::ostringstream oss;
    oss << "rs_mode:" << static_cast<int>(config_.mode)
        << "|scan_time:" << config_.scan_time
        << "|shutter_angle:" << config_.shutter_angle;
    return oss.str();
}

} // namespace
