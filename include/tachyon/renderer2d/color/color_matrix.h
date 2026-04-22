#pragma once

#include "tachyon/renderer2d/color/color_space.h"
#include <cmath>

namespace tachyon::renderer2d {

struct Matrix3x3 {
    float m[9]{};

    static const Matrix3x3& identity() {
        static const Matrix3x3 id{{1, 0, 0, 0, 1, 0, 0, 0, 1}};
        return id;
    }
};

inline Matrix3x3 multiply(const Matrix3x3& a, const Matrix3x3& b) {
    Matrix3x3 result;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result.m[row * 3 + col] = a.m[row * 3 + 0] * b.m[0 * 3 + col] + a.m[row * 3 + 1] * b.m[1 * 3 + col] + a.m[row * 3 + 2] * b.m[2 * 3 + col];
        }
    }
    return result;
}

inline Matrix3x3 invert(const Matrix3x3& matrix) {
    const float a = matrix.m[0], b = matrix.m[1], c = matrix.m[2], d = matrix.m[3], e = matrix.m[4], f = matrix.m[5], g = matrix.m[6], h = matrix.m[7], i = matrix.m[8];
    const float A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g, D = -(b * i - c * h), E = a * i - c * g, F = -(a * h - b * g), G = b * f - c * e, H = -(a * f - c * d), I = a * e - b * d;
    const float det = a * A + b * B + c * C;
    if (std::abs(det) <= 1.0e-8f) return Matrix3x3{};
    const float inv_det = 1.0f / det;
    return Matrix3x3{{A * inv_det, D * inv_det, G * inv_det, B * inv_det, E * inv_det, H * inv_det, C * inv_det, F * inv_det, I * inv_det}};
}

inline Matrix3x3 rgb_to_xyz_matrix(ColorPrimaries primaries) {
    switch (primaries) {
        case ColorPrimaries::Rec709:
        case ColorPrimaries::sRGB:
            return Matrix3x3{{0.4123908f, 0.3575843f, 0.1804808f, 0.2126390f, 0.7151687f, 0.0721923f, 0.0193308f, 0.1191948f, 0.9505322f}};
        case ColorPrimaries::DisplayP3:
            return Matrix3x3{{0.4865709f, 0.2656677f, 0.1982173f, 0.2289746f, 0.6917385f, 0.0792869f, 0.0000000f, 0.0451134f, 1.0439444f}};
        default: return Matrix3x3::identity();
    }
}

inline Matrix3x3 primaries_conversion_matrix(ColorPrimaries source, ColorPrimaries destination) {
    return multiply(invert(rgb_to_xyz_matrix(destination)), rgb_to_xyz_matrix(source));
}

} // namespace tachyon::renderer2d
