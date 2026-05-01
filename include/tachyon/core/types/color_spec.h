#pragma once

#include "tachyon/core/math/vector3.h"
#include <cstdint>
#include <nlohmann/json.hpp>

namespace tachyon {

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] bool operator==(const ColorSpec& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    [[nodiscard]] bool operator!=(const ColorSpec& other) const {
        return !(*this == other);
    }

    [[nodiscard]] math::Vector3 to_vector3() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }
};

inline void to_json(nlohmann::json& j, const ColorSpec& c) {
    j = nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
}

inline void from_json(const nlohmann::json& j, ColorSpec& c) {
    if (j.contains("r") && j.at("r").is_number()) c.r = j.at("r").get<std::uint8_t>();
    if (j.contains("g") && j.at("g").is_number()) c.g = j.at("g").get<std::uint8_t>();
    if (j.contains("b") && j.at("b").is_number()) c.b = j.at("b").get<std::uint8_t>();
    if (j.contains("a") && j.at("a").is_number()) c.a = j.at("a").get<std::uint8_t>();
}

} // namespace tachyon
