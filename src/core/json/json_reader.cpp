#include "tachyon/core/json/json_reader.h"
#include <algorithm>

namespace tachyon {

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_string()) return false;
    out = value.get<std::string>();
    return true;
}

bool read_bool(const json& object, const char* key, bool& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_boolean()) return false;
    out = value.get<bool>();
    return true;
}

bool read_double(const json& object, const char* key, double& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<double>();
    return true;
}

bool read_int64(const json& object, const char* key, std::int64_t& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number_integer()) return false;
    out = value.get<std::int64_t>();
    return true;
}

bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number_integer()) return false;
    out = value.get<std::int64_t>();
    return true;
}

bool read_vector2(const json& value, math::Vector2& out) {
    if (value.is_array()) {
        if (value.size() < 2 || !value[0].is_number() || !value[1].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()) };
        return true;
    }
    if (value.is_object()) {
        if (!value.contains("x") || !value.contains("y") || !value.at("x").is_number() || !value.at("y").is_number()) return false;
        out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float scalar = static_cast<float>(value.get<double>());
        out = {scalar, scalar};
        return true;
    }
    return false;
}

bool read_vector3(const json& value, math::Vector3& out) {
    if (value.is_array()) {
        if (value.size() < 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()), static_cast<float>(value[2].get<double>()) };
        return true;
    }
    if (value.is_object()) {
        if (!value.contains("x") || !value.contains("y") || !value.contains("z") ||
            !value.at("x").is_number() || !value.at("y").is_number() || !value.at("z").is_number()) return false;
        out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()), static_cast<float>(value.at("z").get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float s = static_cast<float>(value.get<double>());
        out = {s, s, s};
        return true;
    }
    return false;
}

bool read_color(const json& value, ColorSpec& out) {
    if (value.is_array()) {
        if (value.size() < 3) return false;
        out.r = static_cast<std::uint8_t>(std::clamp(value[0].get<int>(), 0, 255));
        out.g = static_cast<std::uint8_t>(std::clamp(value[1].get<int>(), 0, 255));
        out.b = static_cast<std::uint8_t>(std::clamp(value[2].get<int>(), 0, 255));
        out.a = (value.size() >= 4) ? static_cast<std::uint8_t>(std::clamp(value[3].get<int>(), 0, 255)) : 255;
        return true;
    }
    if (value.is_object()) {
        if (value.contains("r") && value.contains("g") && value.contains("b")) {
            out.r = static_cast<std::uint8_t>(std::clamp(value.at("r").get<int>(), 0, 255));
            out.g = static_cast<std::uint8_t>(std::clamp(value.at("g").get<int>(), 0, 255));
            out.b = static_cast<std::uint8_t>(std::clamp(value.at("b").get<int>(), 0, 255));
            out.a = (value.contains("a")) ? static_cast<std::uint8_t>(std::clamp(value.at("a").get<int>(), 0, 255)) : 255;
            return true;
        }
    }
    return false;
}

} // namespace tachyon
