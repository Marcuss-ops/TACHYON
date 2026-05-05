#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>

namespace tachyon::registry {

template <typename TSpec>
class TypedRegistry {
public:
    void register_spec(TSpec spec) {
        std::lock_guard<std::mutex> lock(mutex_);
        specs_[spec.id] = std::move(spec);
    }

    [[nodiscard]] bool erase(std::string_view id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return specs_.erase(std::string(id)) > 0U;
    }

    [[nodiscard]] const TSpec* find(std::string_view id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = specs_.find(std::string(id));
        if (it != specs_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * @brief Validates a parameter bag against the schema of a specific entry.
     * @return true if valid or if no schema exists, false otherwise.
     */
    [[nodiscard]] bool validate(std::string_view id, const ParameterBag& bag) const {
        const auto* spec = find(id);
        if (!spec) return false;

        // Use constexpr if to check if TSpec has a 'schema' member
        if constexpr (requires(TSpec s) { s.schema; }) {
            for (const auto& def : spec->schema.params) {
                if (def.required && !bag.contains(def.id)) {
                    return false;
                }
                
                // Optional: add type/range checks here
                if (bag.contains(def.id)) {
                    // Type checking could be added here if needed
                }
            }
        }
        return true;
    }

    [[nodiscard]] std::vector<std::string> list_ids() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        ids.reserve(specs_.size());
        for (const auto& [id, _] : specs_) {
            ids.push_back(id);
        }
        return ids;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return specs_.size();
    }

    [[nodiscard]] bool contains(std::string_view id) const {
        return find(id) != nullptr;
    }

    TypedRegistry() = default;

private:
    mutable std::mutex mutex_;
    std::map<std::string, TSpec, std::less<>> specs_;
};

} // namespace tachyon::registry
