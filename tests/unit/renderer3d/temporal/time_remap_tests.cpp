#include "tachyon/renderer3d/temporal/time_remap_curve.h"
#include <cassert>
#include <string>

void test_time_remap_hold_mode() {
    tachyon::renderer3d::temporal::TimeRemapParams params{
        tachyon::renderer3d::temporal::TimeRemapMode::kHold,
        180.0f, 0.0f, 8, nullptr
    };
    tachyon::renderer3d::temporal::TimeRemapCurve curve(params);
    float result = curve.evaluate(0.15f, 0.1f); // frame duration 0.1s, target 0.15s
    assert(std::abs(result - 0.1f) < 1e-6f); // floor(0.15/0.1)=1 *0.1=0.1
}

void test_time_remap_optical_flow_fallback() {
    tachyon::renderer3d::temporal::TimeRemapParams params{
        tachyon::renderer3d::temporal::TimeRemapMode::kOpticalFlow,
        180.0f, 0.0f, 8,
        [](float conf) { return conf < 0.5f ? 0.0f : 1.0f; }
    };
    tachyon::renderer3d::temporal::TimeRemapCurve curve(params);
    float result = curve.evaluate(0.2f, 0.1f, 0.3f); // low confidence
    assert(std::abs(result - 0.0f) < 1e-6f);
}

void test_time_remap_cache_identity() {
    tachyon::renderer3d::temporal::TimeRemapParams params{
        tachyon::renderer3d::temporal::TimeRemapMode::kBlend,
        90.0f, 0.25f, 4, nullptr
    };
    tachyon::renderer3d::temporal::TimeRemapCurve curve(params);
    std::string id = curve.cache_identity();
    assert(id.find("remap_mode:1") != std::string::npos);
}

bool run_time_remap_tests() {
    test_time_remap_hold_mode();
    test_time_remap_optical_flow_fallback();
    test_time_remap_cache_identity();
    return true;
}
