#pragma once

#include <cstdint>

namespace tachyon {

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] bool operator==(const ColorSpec& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    [[nodiscard]] bool operator!=(const ColorSpec& other) const {
        return !(*this == other);
    }

};

} // namespace tachyon
