#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <cstdint>
#include <vector>

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/types/color_spec.h"

namespace tachyon {

using json = nlohmann::json;

bool read_string(const json& object, const char* key, std::string& out);
bool read_bool(const json& object, const char* key, bool& out);
bool read_double(const json& object, const char* key, double& out);
bool read_int64(const json& object, const char* key, std::int64_t& out);
bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out);

template <typename T>
bool read_number(const json& object, const char* key, T& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<T>();
    return true;
}

template <typename T>
bool read_number(const json& object, const char* key, std::optional<T>& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<T>();
    return true;
}

} // namespace tachyon
