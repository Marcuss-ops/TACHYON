#include "tachyon/renderer3d/temporal/frame_blend_mode.h"
#include <cassert>
#include <string>

void test_frame_blend_hold() {
    tachyon::renderer3d::temporal::FrameBlendParams params{
        tachyon::renderer3d::temporal::FrameBlendMode::kHold, 1
    };
    tachyon::renderer3d::temporal::FrameBlender blender(params);
    assert(blender.blend_factor(0) == 1.0f);
    assert(blender.blend_factor(1) == 0.0f);
}

void test_frame_blend_linear() {
    tachyon::renderer3d::temporal::FrameBlendParams params{
        tachyon::renderer3d::temporal::FrameBlendMode::kLinear, 2
    };
    tachyon::renderer3d::temporal::FrameBlender blender(params);
    assert(std::abs(blender.blend_factor(0) - 1.0f) < 1e-6f);
    assert(std::abs(blender.blend_factor(1) - 0.5f) < 1e-6f);
}

void test_frame_blend_cache_identity() {
    tachyon::renderer3d::temporal::FrameBlendParams params{
        tachyon::renderer3d::temporal::FrameBlendMode::kOpticalFlow, 4
    };
    tachyon::renderer3d::temporal::FrameBlender blender(params);
    std::string id = blender.cache_identity();
    assert(id.find("blend_mode:2") != std::string::npos);
}

int main() {
    test_frame_blend_hold();
    test_frame_blend_linear();
    test_frame_blend_cache_identity();
    return 0;
}
