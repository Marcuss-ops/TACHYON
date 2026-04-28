#pragma once
#include <vector>
#include <cstddef>

namespace tachyon::tracker {

struct Matrix2x2 {
    float m[2][2];

    float det() const {
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    }

    Matrix2x2 inverse() const {
        float d = det();
        Matrix2x2 inv{};
        if (std::abs(d) > 1e-6f) {
            inv.m[0][0] = m[1][1] / d;
            inv.m[0][1] = -m[0][1] / d;
            inv.m[1][0] = -m[1][0] / d;
            inv.m[1][1] = m[0][0] / d;
        }
        return inv;
    }
};

struct TranslationEstimate {
    bool found{false};
    int dx{0};
    int dy{0};
    float confidence{0.0f};
};

} // namespace tachyon::tracker
