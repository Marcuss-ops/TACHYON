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
                
                if (const auto* val_ptr = bag.get_raw(def.id)) {
                    const auto& val = *val_ptr;
                    
                    // Type checking: ensure the variant index matches the default value's index
                    // This enforces that the type in the bag matches the expected type in the schema
                    if (val.index() != def.default_value.index()) {
                        return false;
                    }

                    // Range checking for numeric types
                    if (def.min_value.has_value() || def.max_value.has_value()) {
                        double numeric_val = 0.0;
                        bool is_numeric = false;
                        
                        if (const auto* f = std::get_if<float>(&val)) { numeric_val = *f; is_numeric = true; }
                        else if (const auto* d = std::get_if<double>(&val)) { numeric_val = *d; is_numeric = true; }
                        else if (const auto* i = std::get_if<int>(&val)) { numeric_val = *i; is_numeric = true; }
                        
                        if (is_numeric) {
                            if (def.min_value && numeric_val < *def.min_value) return false;
                            if (def.max_value && numeric_val > *def.max_value) return false;
                        }
                    }
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

    [[nodiscard]] std::vector<std::string> list_by_category(std::string_view category) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        for (const auto& [id, spec] : specs_) {
            if constexpr (requires(TSpec s) { s.metadata; }) {
                if (spec.metadata.category == category) {
                    ids.push_back(id);
                }
            }
        }
        return ids;
    }

    [[nodiscard]] std::vector<std::string> list_by_tag(std::string_view tag) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        for (const auto& [id, spec] : specs_) {
            if constexpr (requires(TSpec s) { s.metadata; }) {
                for (const auto& t : spec.metadata.tags) {
                    if (t == tag) {
                        ids.push_back(id);
                        break;
                    }
                }
            }
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
