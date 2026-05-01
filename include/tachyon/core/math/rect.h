#pragma once

namespace tachyon::math {

/**
 * @brief A simple rectangle defined by x, y, width, and height.
 */
struct RectF {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    static constexpr RectF zero() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    static constexpr RectF unit() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
};

struct RectI {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

} // namespace tachyon::math
