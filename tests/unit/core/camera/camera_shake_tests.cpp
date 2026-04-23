#include "tachyon/core/camera/camera_shake.h"
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

bool nearly_equal(float a, float b, float eps = 1e-4f) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_camera_shake_tests() {
    g_failures = 0;

    using namespace tachyon::camera;

    // Inactive shake returns zero displacement
    {
        CameraShake shake;
        shake.amplitude_position = 0.0f;
        shake.amplitude_rotation = 0.0f;
        check_true(!shake.is_active(), "zero amplitude is inactive");

        auto p = shake.evaluate_position(1.0f);
        check_true(nearly_equal(p.x, 0.0f), "inactive position x is 0");
        check_true(nearly_equal(p.y, 0.0f), "inactive position y is 0");
        check_true(nearly_equal(p.z, 0.0f), "inactive position z is 0");

        auto r = shake.evaluate_rotation(1.0f);
        check_true(nearly_equal(r.x, 0.0f), "inactive rotation x is 0");
    }

    // Determinism: same seed + same time = same value
    {
        CameraShake shake;
        shake.seed = 42;
        shake.amplitude_position = 10.0f;
        shake.frequency = 2.0f;
        shake.roughness = 0.5f;

        auto p1 = shake.evaluate_position(1.5f);
        auto p2 = shake.evaluate_position(1.5f);
        check_true(nearly_equal(p1.x, p2.x), "position deterministic x");
        check_true(nearly_equal(p1.y, p2.y), "position deterministic y");
        check_true(nearly_equal(p1.z, p2.z), "position deterministic z");

        auto r1 = shake.evaluate_rotation(1.5f);
        auto r2 = shake.evaluate_rotation(1.5f);
        check_true(nearly_equal(r1.x, r2.x), "rotation deterministic x");
        check_true(nearly_equal(r1.y, r2.y), "rotation deterministic y");
        check_true(nearly_equal(r1.z, r2.z), "rotation deterministic z");
    }

    // Different seeds produce different values
    {
        CameraShake shake1;
        shake1.seed = 1;
        shake1.amplitude_position = 10.0f;
        shake1.frequency = 1.0f;

        CameraShake shake2;
        shake2.seed = 2;
        shake2.amplitude_position = 10.0f;
        shake2.frequency = 1.0f;

        auto p1 = shake1.evaluate_position(1.0f);
        auto p2 = shake2.evaluate_position(1.0f);
        // Very unlikely to be exactly equal with different seeds
        bool different = (std::abs(p1.x - p2.x) > 1e-6f) ||
                         (std::abs(p1.y - p2.y) > 1e-6f) ||
                         (std::abs(p1.z - p2.z) > 1e-6f);
        check_true(different, "different seeds produce different noise");
    }

    // Amplitude bounds: values within [-amplitude, +amplitude]
    {
        CameraShake shake;
        shake.seed = 123;
        shake.amplitude_position = 5.0f;
        shake.frequency = 1.0f;
        shake.roughness = 1.0f;

        for (float t = 0.0f; t <= 2.0f; t += 0.1f) {
            auto p = shake.evaluate_position(t);
            check_true(std::abs(p.x) <= 5.0f + 1e-5f, "position x within amplitude");
            check_true(std::abs(p.y) <= 5.0f + 1e-5f, "position y within amplitude");
            check_true(std::abs(p.z) <= 5.0f + 1e-5f, "position z within amplitude");
        }
    }

    // Rotation amplitude bounds
    {
        CameraShake shake;
        shake.seed = 456;
        shake.amplitude_rotation = 15.0f;
        shake.frequency = 1.0f;
        shake.roughness = 1.0f;

        for (float t = 0.0f; t <= 2.0f; t += 0.1f) {
            auto r = shake.evaluate_rotation(t);
            check_true(std::abs(r.x) <= 15.0f + 1e-5f, "rotation x within amplitude");
            check_true(std::abs(r.y) <= 15.0f + 1e-5f, "rotation y within amplitude");
            check_true(std::abs(r.z) <= 15.0f + 1e-5f, "rotation z within amplitude");
        }
    }

    // Active with only rotation
    {
        CameraShake shake;
        shake.amplitude_rotation = 5.0f;
        check_true(shake.is_active(), "rotation-only is active");
    }

    // Active with only position
    {
        CameraShake shake;
        shake.amplitude_position = 5.0f;
        check_true(shake.is_active(), "position-only is active");
    }

    return g_failures == 0;
}
