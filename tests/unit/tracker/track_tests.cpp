#include "tachyon/tracker/track.h"
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

bool run_track_tests() {
    g_failures = 0;

    using namespace tachyon::tracker;

    // Empty track returns nullopt
    {
        Track t;
        check_true(!t.position_at(0.0f).has_value(), "empty track returns nullopt");
        check_true(!t.affine_at(0.0f).has_value(), "empty track affine returns nullopt");
        check_true(!t.homography_at(0.0f).has_value(), "empty track homography returns nullopt");
    }

    // Single sample: before, at, after
    {
        Track t;
        t.samples = {{0.0f, {10.0f, 20.0f}, 1.0f}};
        auto p0 = t.position_at(0.0f);
        check_true(p0.has_value(), "single sample at time exists");
        check_true(nearly_equal(p0->x, 10.0f), "single sample x");
        check_true(nearly_equal(p0->y, 20.0f), "single sample y");

        auto p_before = t.position_at(-1.0f);
        check_true(p_before.has_value(), "before first sample clamps");
        check_true(nearly_equal(p_before->x, 10.0f), "clamp before x");

        auto p_after = t.position_at(5.0f);
        check_true(p_after.has_value(), "after last sample clamps");
        check_true(nearly_equal(p_after->x, 10.0f), "clamp after x");
    }

    // Linear interpolation between two samples
    {
        Track t;
        t.samples = {
            {0.0f, {0.0f, 0.0f}, 1.0f},
            {1.0f, {10.0f, 20.0f}, 1.0f}
        };
        auto p = t.position_at(0.5f);
        check_true(p.has_value(), "interp at 0.5 exists");
        check_true(nearly_equal(p->x, 5.0f), "interp x = 5");
        check_true(nearly_equal(p->y, 10.0f), "interp y = 10");
    }

    // Binary search with many samples
    {
        Track t;
        for (int i = 0; i <= 10; ++i) {
            t.samples.push_back({static_cast<float>(i), {static_cast<float>(i * 2), static_cast<float>(i * 3)}, 1.0f});
        }
        auto p = t.position_at(7.5f);
        check_true(p.has_value(), "many samples interp exists");
        check_true(nearly_equal(p->x, 15.0f), "many samples x = 15");
        check_true(nearly_equal(p->y, 22.5f), "many samples y = 22.5");
    }

    // Affine hold interpolation
    {
        Track t;
        t.samples = {
            {0.0f, {0.0f, 0.0f}, 1.0f},
            {1.0f, {0.0f, 0.0f}, 1.0f}
        };
        t.affine_per_sample = std::vector<std::vector<float>>{
            {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
            {2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f}
        };
        auto a0 = t.affine_at(0.0f);
        check_true(a0.has_value(), "affine at 0 exists");
        check_true(nearly_equal((*a0)[0], 1.0f), "affine at 0 scale x");

        auto a_mid = t.affine_at(0.5f);
        check_true(a_mid.has_value(), "affine at 0.5 exists");
        check_true(nearly_equal((*a_mid)[0], 1.0f), "affine hold at 0.5 returns first");

        auto a1 = t.affine_at(1.0f);
        check_true(a1.has_value(), "affine at 1 exists");
        check_true(nearly_equal((*a1)[0], 2.0f), "affine at 1 scale x");
    }

    // Homography hold interpolation
    {
        Track t;
        t.samples = {
            {0.0f, {0.0f, 0.0f}, 1.0f},
            {1.0f, {0.0f, 0.0f}, 1.0f}
        };
        t.homography_per_sample = std::vector<std::vector<float>>{
            {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
            {2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f}
        };
        auto h0 = t.homography_at(0.0f);
        check_true(h0.has_value(), "homography at 0 exists");
        check_true(nearly_equal((*h0)[0], 1.0f), "homography at 0 [0]");

        auto h_mid = t.homography_at(0.5f);
        check_true(h_mid.has_value(), "homography at 0.5 exists");
        check_true(nearly_equal((*h_mid)[0], 1.0f), "homography hold at 0.5");
    }

    return g_failures == 0;
}
