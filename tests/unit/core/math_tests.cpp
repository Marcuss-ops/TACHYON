#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/matrix3x3.h"
#include "tachyon/core/math/transform2.h"

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

bool nearly_equal_vec2(const tachyon::math::Vector2& a, const tachyon::math::Vector2& b, float eps = kEps) {
    return nearly_equal(a.x, b.x, eps) && nearly_equal(a.y, b.y, eps);
}

} // namespace

bool run_math_tests() {
    using namespace tachyon::math;

    std::cout << "Running 2D Math Tests..." << std::endl;

    {
        Matrix3x3 m = Matrix3x3::make_translation({10.0f, 20.0f});
        Vector2 p = {0.0f, 0.0f};
        Vector2 p_prime = m.transform_point(p);
        check_true(nearly_equal_vec2(p_prime, {10.0f, 20.0f}), "Matrix3x3 translation (0,0) -> (10,20)");
    }

    {
        Matrix3x3 m = Matrix3x3::make_rotation(3.14159265f * 0.5f); // 90 deg
        Vector2 p = {1.0f, 0.0f};
        Vector2 p_prime = m.transform_point(p);
        check_true(nearly_equal_vec2(p_prime, {0.0f, 1.0f}), "Matrix3x3 rotation (1,0) 90deg -> (0,1)");
    }

    {
        Matrix3x3 m = Matrix3x3::make_scale(2.0f, 3.0f);
        Vector2 p = {1.0f, 1.0f};
        Vector2 p_prime = m.transform_point(p);
        check_true(nearly_equal_vec2(p_prime, {2.0f, 3.0f}), "Matrix3x3 scale (1,1) x2,y3 -> (2,3)");
    }

    {
        Transform2 t;
        t.position = {10.0f, 20.0f};
        t.rotation_rad = 3.14159265f * 0.5f;
        t.scale = {2.0f, 2.0f};
        t.anchor_point = {1.0f, 1.0f};

        Matrix3x3 m = t.to_matrix();
        // Step 1: Anchor point -> (0,0) from (1,1)
        // Step 2: Scale x2 -> (0,0)
        // Step 3: Rotate 90deg -> (0,0)
        // Step 4: Translate -> (10,20)
        Vector2 p = {1.0f, 1.0f};
        Vector2 p_prime = m.transform_point(p);
        check_true(nearly_equal_vec2(p_prime, {10.0f, 20.0f}), "Transform2 full TRS with anchor point");
    }

    if (g_failures == 0) {
        std::cout << "All 2D math tests passed." << std::endl;
    }

    return g_failures == 0;
}
