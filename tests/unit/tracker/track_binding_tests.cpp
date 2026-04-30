#include "tachyon/tracker/track_binding.h"
#include "tachyon/tracker/track.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/transform2.h"
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

bool run_track_binding_tests() {
    g_failures = 0;

    using namespace tachyon::tracker;
    using namespace tachyon::scene;

    // Position binding
    {
        Track t;
        t.samples = {
            {0.0f, {10.0f, 20.0f}, 1.0f},
            {1.0f, {30.0f, 40.0f}, 1.0f}
        };
        TrackBinding binding(t, TrackBindingTarget::Position);
        EvaluatedLayerState layer;
        layer.local_transform = tachyon::math::Transform2::identity();

        binding.apply(0.0f, layer);
        check_true(nearly_equal(layer.local_transform.position.x, 10.0f), "position binding x at 0");
        check_true(nearly_equal(layer.local_transform.position.y, 20.0f), "position binding y at 0");

        binding.apply(0.5f, layer);
        check_true(nearly_equal(layer.local_transform.position.x, 20.0f), "position binding x at 0.5");
        check_true(nearly_equal(layer.local_transform.position.y, 30.0f), "position binding y at 0.5");
    }

    // Rotation binding from affine
    {
        Track t;
        t.samples = {{0.0f, {0.0f, 0.0f}, 1.0f}};
        t.affine_per_sample = std::vector<std::vector<float>>{
            {0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f} // 90 deg rotation_rad
        };
        TrackBinding binding(t, TrackBindingTarget::Rotation);
        EvaluatedLayerState layer;
        layer.local_transform = tachyon::math::Transform2::identity();

        binding.apply(0.0f, layer);
        // rotation_rad angle should be ~90 degrees in radians
        check_true(std::abs(layer.local_transform.rotation_rad - 1.5708f) < 0.01f, "rotation_rad binding from affine");
    }

    // Scale binding from affine
    {
        Track t;
        t.samples = {{0.0f, {0.0f, 0.0f}, 1.0f}};
        t.affine_per_sample = std::vector<std::vector<float>>{
            {2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f}
        };
        TrackBinding binding(t, TrackBindingTarget::Scale);
        EvaluatedLayerState layer;
        layer.local_transform = tachyon::math::Transform2::identity();

        binding.apply(0.0f, layer);
        check_true(nearly_equal(layer.local_transform.scale.x, 2.0f), "scale binding x");
        check_true(nearly_equal(layer.local_transform.scale.y, 3.0f), "scale binding y");
    }

    // CornerPin binding delegates to PlanarTrack (placeholder: no homography data)
    {
        Track t;
        t.samples = {{0.0f, {0.0f, 0.0f}, 1.0f}};
        TrackBinding binding(t, TrackBindingTarget::CornerPin);
        EvaluatedLayerState layer;
        layer.local_transform = tachyon::math::Transform2::identity();

        binding.apply(0.0f, layer);
        // Without homography data, layer should be left unchanged
        check_true(nearly_equal(layer.local_transform.position.x, 0.0f), "cornerpin no-op position x");
    }

    // Missing track data leaves layer unchanged
    {
        Track t; // empty
        TrackBinding binding(t, TrackBindingTarget::Position);
        EvaluatedLayerState layer;
        layer.local_transform.position.x = 99.0f;

        binding.apply(0.0f, layer);
        check_true(nearly_equal(layer.local_transform.position.x, 99.0f), "missing data leaves layer unchanged");
    }

    return g_failures == 0;
}
