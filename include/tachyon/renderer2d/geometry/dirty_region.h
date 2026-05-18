#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace tachyon::renderer2d {

struct IntRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    [[nodiscard]] bool empty() const noexcept {
        return width <= 0 || height <= 0;
    }

    [[nodiscard]] int right() const noexcept {
        return x + width;
    }

    [[nodiscard]] int bottom() const noexcept {
        return y + height;
    }

    [[nodiscard]] bool intersects(const IntRect& other) const noexcept {
        if (empty() || other.empty()) return false;
        return x < other.right() &&
               right() > other.x &&
               y < other.bottom() &&
               bottom() > other.y;
    }

    [[nodiscard]] IntRect intersection(const IntRect& other) const noexcept {
        const int nx = std::max(x, other.x);
        const int ny = std::max(y, other.y);
        const int nr = std::min(right(), other.right());
        const int nb = std::min(bottom(), other.bottom());

        if (nr <= nx || nb <= ny) return {};
        return {nx, ny, nr - nx, nb - ny};
    }

    [[nodiscard]] IntRect united(const IntRect& other) const noexcept {
        if (empty()) return other;
        if (other.empty()) return *this;

        const int nx = std::min(x, other.x);
        const int ny = std::min(y, other.y);
        const int nr = std::max(right(), other.right());
        const int nb = std::max(bottom(), other.bottom());

        return {nx, ny, nr - nx, nb - ny};
    }

    [[nodiscard]] IntRect expanded(int amount) const noexcept {
        if (empty()) return {};
        return {
            x - amount,
            y - amount,
            width + amount * 2,
            height + amount * 2
        };
    }

    [[nodiscard]] IntRect clipped_to(const IntRect& bounds) const noexcept {
        return intersection(bounds);
    }
    
    bool operator==(const IntRect& other) const noexcept {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
};

class DirtyRegion {
public:
    void add(IntRect rect);
    void add_union(IntRect rect);
    void clear();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<IntRect>& rects() const noexcept;
    [[nodiscard]] IntRect bounds() const noexcept;

    void optimize();

private:
    std::vector<IntRect> rects_;
};

} // namespace tachyon::renderer2d