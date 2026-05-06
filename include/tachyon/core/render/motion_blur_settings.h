#pragma once

namespace tachyon {

enum class MotionBlurWeightCurve { kBox, kTriangle, kGaussian };

struct MotionBlurConfig {
    bool enabled{false};
    int samples{8};
    double shutter_angle{180.0};
    double shutter_phase{-90.0};
    MotionBlurWeightCurve weight_curve{MotionBlurWeightCurve::kBox};
    bool enable_camera_blur{true};
    bool enable_object_blur{true};
};

} // namespace tachyon
