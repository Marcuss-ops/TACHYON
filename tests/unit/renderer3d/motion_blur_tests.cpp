#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/transform3.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;
constexpr float kEps = 1e-4f;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = kEps) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_motion_blur_tests() {
    using namespace tachyon::renderer3d;
    using namespace tachyon::math;

    g_failures = 0;

    {
        MotionBlurRenderer renderer;
        check_true(!renderer.is_enabled(), "default motion blur is disabled");
        check_true(renderer.get_subframe_count() == 1, "default subframe count is 1");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 4;
        config.shutter_angle = 180.0;
        config.shutter_phase = 0.0;
        config.curve = "box";

        MotionBlurRenderer renderer(config);
        check_true(renderer.is_enabled(), "motion blur enabled with samples > 1");
        check_true(renderer.get_subframe_count() == 4, "subframe count matches config samples");

        auto times = renderer.compute_subframe_times(1.0, 1.0 / 30.0);
        check_true(times.size() == 4, "subframe times count matches samples");
        check_true(nearly_equal(static_cast<float>(times.front()), 1.0f - (1.0f / 30.0f) * 0.25f, 1e-3f), "first subframe is before frame time");
        check_true(nearly_equal(static_cast<float>(times.back()), 1.0f + (1.0f / 30.0f) * 0.25f, 1e-3f), "last subframe is after frame time");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 1;
        MotionBlurRenderer renderer(config);
        check_true(!renderer.is_enabled(), "motion blur disabled with 1 sample");

        auto times = renderer.compute_subframe_times(2.0, 1.0 / 24.0);
        check_true(times.size() == 1, "single sample returns one time");
        check_true(nearly_equal(static_cast<float>(times[0]), 2.0f), "single sample time equals frame time");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 8;
        config.curve = "box";
        MotionBlurRenderer renderer(config);

        float w0 = renderer.evaluate_weight(0, 8);
        float w4 = renderer.evaluate_weight(4, 8);
        float w7 = renderer.evaluate_weight(7, 8);
        check_true(nearly_equal(w0, 1.0f), "box curve weight is uniform");
        check_true(nearly_equal(w4, 1.0f), "box curve center weight is uniform");
        check_true(nearly_equal(w7, 1.0f), "box curve last weight is uniform");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 5;
        config.curve = "triangle";
        MotionBlurRenderer renderer(config);

        float w0 = renderer.evaluate_weight(0, 5);
        float w2 = renderer.evaluate_weight(2, 5);
        float w4 = renderer.evaluate_weight(4, 5);
        check_true(nearly_equal(w0, 0.0f, 1e-3f), "triangle curve edge weight is zero");
        check_true(nearly_equal(w2, 1.0f, 1e-3f), "triangle curve center weight is one");
        check_true(nearly_equal(w4, 0.0f, 1e-3f), "triangle curve last weight is zero");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 5;
        config.curve = "gaussian";
        MotionBlurRenderer renderer(config);

        float w0 = renderer.evaluate_weight(0, 5);
        float w2 = renderer.evaluate_weight(2, 5);
        float w4 = renderer.evaluate_weight(4, 5);
        check_true(w2 > w0, "gaussian center weight is higher than edge");
        check_true(nearly_equal(w0, w4), "gaussian weights are symmetric");
    }

    {
        Matrix4x4 prev = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));
        Matrix4x4 curr = compose_trs(Vector3(10.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));

        auto velocity = MotionBlurRenderer::compute_object_velocity(prev, curr, 1.0);
        check_true(nearly_equal(velocity.linear_velocity.x, 10.0f), "linear velocity x matches displacement");
        check_true(nearly_equal(velocity.linear_velocity.y, 0.0f), "linear velocity y is zero");
        check_true(nearly_equal(velocity.linear_velocity.z, 0.0f), "linear velocity z is zero");
        check_true(nearly_equal(velocity.angular_velocity.length(), 0.0f), "angular velocity is zero for no rotation");
    }

    {
        Matrix4x4 prev = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));
        Matrix4x4 curr = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::from_axis_angle(Vector3::up(), 3.14159265f), Vector3(1.0f, 1.0f, 1.0f));

        auto velocity = MotionBlurRenderer::compute_object_velocity(prev, curr, 1.0);
        check_true(nearly_equal(velocity.angular_velocity.length(), 3.14159265f, 1e-3f), "angular velocity matches rotation rate");
    }

    {
        Matrix4x4 prev = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));
        Matrix4x4 curr = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));

        auto velocity = MotionBlurRenderer::compute_object_velocity(prev, curr, 0.0);
        check_true(nearly_equal(velocity.linear_velocity.length(), 0.0f), "zero delta time produces zero linear velocity");
        check_true(nearly_equal(velocity.angular_velocity.length(), 0.0f), "zero delta time produces zero angular velocity");
    }

    {
        Matrix4x4 a = compose_trs(Vector3(0.0f, 0.0f, 0.0f), Quaternion::identity(), Vector3(1.0f, 1.0f, 1.0f));
        Matrix4x4 b = compose_trs(Vector3(10.0f, 20.0f, 30.0f), Quaternion::identity(), Vector3(2.0f, 2.0f, 2.0f));

        Matrix4x4 mid = MotionBlurRenderer::interpolate_world_matrix(a, b, 0.5f);
        Transform3 mid_trs = mid.to_transform();
        check_true(nearly_equal(mid_trs.position.x, 5.0f), "interpolated position x is midpoint");
        check_true(nearly_equal(mid_trs.position.y, 10.0f), "interpolated position y is midpoint");
        check_true(nearly_equal(mid_trs.position.z, 15.0f), "interpolated position z is midpoint");
        check_true(nearly_equal(mid_trs.scale.x, 1.5f), "interpolated scale x is midpoint");
    }

    {
        MotionBlurRenderer::MotionBlurConfig config;
        config.enabled = true;
        config.samples = 4;
        config.shutter_angle = 180.0;
        MotionBlurRenderer renderer(config);
        renderer.set_quality_tier("final");
        check_true(renderer.get_quality_tier() == "final", "quality tier setter/getter works");
    }

    return g_failures == 0;
}
