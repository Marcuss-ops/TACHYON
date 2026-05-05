#pragma once

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

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
