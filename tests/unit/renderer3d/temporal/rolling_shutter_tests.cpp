#include "tachyon/renderer3d/temporal/rolling_shutter.h"
#include <cassert>
#include <string>
#include <vector>

void test_rolling_shutter_disabled() {
    tachyon::renderer3d::temporal::RollingShutterConfig config;
    config.mode = tachyon::renderer3d::temporal::RollingShutterMode::kNone;
    tachyon::renderer3d::temporal::RollingShutterEffect rs(config);
    assert(!rs.is_enabled());
}

void test_rolling_shutter_exposure_offset() {
    tachyon::renderer3d::temporal::RollingShutterConfig config;
    config.mode = tachyon::renderer3d::temporal::RollingShutterMode::kTopToBottom;
    config.scan_time = 0.1f;
    tachyon::renderer3d::temporal::RollingShutterEffect rs(config);
    assert(rs.is_enabled());
    float offset = rs.exposure_offset(0.5f);
    assert(std::abs(offset - 0.05f) < 1e-6f);
}

void test_rolling_shutter_cache_identity() {
    tachyon::renderer3d::temporal::RollingShutterConfig config;
    config.mode = tachyon::renderer3d::temporal::RollingShutterMode::kLeftToRight;
    config.scan_time = 0.2f;
    tachyon::renderer3d::temporal::RollingShutterEffect rs(config);
    std::string id = rs.cache_identity();
    assert(id.find("rs_mode:3") != std::string::npos);
}

int main() {
    test_rolling_shutter_disabled();
    test_rolling_shutter_exposure_offset();
    test_rolling_shutter_cache_identity();
    return 0;
}
