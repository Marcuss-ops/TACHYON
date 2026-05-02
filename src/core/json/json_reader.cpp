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

} // namespace tachyon
