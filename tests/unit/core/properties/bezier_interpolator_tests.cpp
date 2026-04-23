#include "tachyon/core/properties/bezier_interpolator.h"
#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = 1e-5f) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_bezier_interpolator_tests() {
    g_failures = 0;

    using namespace tachyon;
    using namespace tachyon::properties;
    using namespace tachyon::animation;

    // Empty keyframes: evaluate returns 0 (or first value if single)
    {
        std::vector<ScalarKeyframe> kfs;
        float v = BezierInterpolator::evaluate(kfs, 0.5f);
        check_true(nearly_equal(v, 0.0f), "empty keyframes evaluate to 0");
    }

    // Single keyframe: always returns its value
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 5.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Hold}
        };
        float v = BezierInterpolator::evaluate(kfs, 0.0f);
        check_true(nearly_equal(v, 5.0f), "single keyframe at time");
        v = BezierInterpolator::evaluate(kfs, 10.0f);
        check_true(nearly_equal(v, 5.0f), "single keyframe after time");
        v = BezierInterpolator::evaluate(kfs, -5.0f);
        check_true(nearly_equal(v, 5.0f), "single keyframe before time");
    }

    // Hold interpolation
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Hold},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Hold}
        };
        float v = BezierInterpolator::evaluate(kfs, 0.5f);
        check_true(nearly_equal(v, 0.0f), "hold at 0.5 returns first value");
        v = BezierInterpolator::evaluate(kfs, 0.0f);
        check_true(nearly_equal(v, 0.0f), "hold at start");
        v = BezierInterpolator::evaluate(kfs, 1.0f);
        check_true(nearly_equal(v, 10.0f), "hold at next keyframe");
    }

    // Linear interpolation
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        float v = BezierInterpolator::evaluate(kfs, 0.5f);
        check_true(nearly_equal(v, 5.0f), "linear at 0.5");
        v = BezierInterpolator::evaluate(kfs, 0.25f);
        check_true(nearly_equal(v, 2.5f), "linear at 0.25");
        v = BezierInterpolator::evaluate(kfs, 0.75f);
        check_true(nearly_equal(v, 7.5f), "linear at 0.75");
    }

    // Before first keyframe clamps
    {
        std::vector<ScalarKeyframe> kfs = {
            {1.0f, 5.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {2.0f, 15.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        float v = BezierInterpolator::evaluate(kfs, 0.0f);
        check_true(nearly_equal(v, 5.0f), "clamp before first");
    }

    // After last keyframe clamps
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        float v = BezierInterpolator::evaluate(kfs, 5.0f);
        check_true(nearly_equal(v, 10.0f), "clamp after last");
    }

    // Validation: sorted keyframes pass
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        std::string error;
        check_true(BezierInterpolator::validate(kfs, error), "sorted keyframes valid");
    }

    // Validation: duplicate times fail
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {0.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        std::string error;
        check_true(!BezierInterpolator::validate(kfs, error), "duplicate times invalid");
    }

    // Validation: unsorted keyframes fail
    {
        std::vector<ScalarKeyframe> kfs = {
            {1.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {0.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        std::string error;
        check_true(!BezierInterpolator::validate(kfs, error), "unsorted keyframes invalid");
    }

    // Validation: bezier with invalid control points fails
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Bezier},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Bezier}
        };
        // Set invalid bezier control points (outside [0,1])
        kfs[0].bezier = animation::CubicBezierEasing(-0.1f, 0.0f, 1.0f, 1.0f);
        std::string error;
        check_true(!BezierInterpolator::validate(kfs, error), "invalid bezier control points");
    }

    // Value to speed graph: constant value -> zero speed
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 5.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Hold},
            {1.0f, 5.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Hold}
        };
        auto speed = BezierInterpolator::value_to_speed_graph(kfs);
        check_true(!speed.empty(), "speed graph not empty");
        for (const auto& s : speed) {
            check_true(nearly_equal(s.value, 0.0f), "constant value -> zero speed");
        }
    }

    // Speed to value graph: integration reconstructs original
    {
        std::vector<ScalarKeyframe> kfs = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear},
            {1.0f, 10.0f, 0.0f, 0.0f, 0.333f, 0.333f, InterpolationType::Linear}
        };
        auto speed = BezierInterpolator::value_to_speed_graph(kfs);
        auto reconstructed = BezierInterpolator::speed_to_value_graph(speed);
        check_true(!reconstructed.empty(), "reconstructed not empty");
        if (!reconstructed.empty()) {
            check_true(nearly_equal(reconstructed.front().value, kfs.front().value), "reconstructed first value");
        }
    }

    return g_failures == 0;
}
