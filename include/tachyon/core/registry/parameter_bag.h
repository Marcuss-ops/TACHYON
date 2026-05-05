#pragma once

#include "tachyon/core/registry/parameter_schema.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace tachyon::registry {

class ParameterBag {
public:
    void set(std::string key, ParameterValue value) {
        values_[std::move(key)] = std::move(value);
    }

    template <typename T>
    [[nodiscard]] std::optional<T> get(std::string_view key) const {
        auto it = values_.find(std::string(key));
        if (it == values_.end()) {
            return std::nullopt;
        }
        if (const auto* value = std::get_if<T>(&it->second)) {
            return *value;
        }
        return std::nullopt;
    }

    template <typename T>
    [[nodiscard]] T get_or(std::string_view key, T fallback) const {
        auto value = get<T>(key);
        return value.has_value() ? *value : fallback;
    }

    [[nodiscard]] bool contains(std::string_view key) const {
        return values_.find(std::string(key)) != values_.end();
    }

    [[nodiscard]] bool empty() const noexcept { return values_.empty(); }

private:
    std::unordered_map<std::string, ParameterValue> values_;
};

} // namespace tachyon::registry
