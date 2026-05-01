#pragma once

#include <list>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

namespace tachyon::media {

/**
 * @brief A generic LRU cache that respects a total memory budget.
 * 
 * @tparam T The type of cached items.
 */
template <typename K, typename T>
class ResourceCache {
public:
    using CostFunc = std::function<std::size_t(const T&)>;

    ResourceCache(std::size_t budget_bytes, CostFunc cost_func)
        : m_budget_bytes(budget_bytes), m_cost_func(cost_func) {}

    void set_budget(std::size_t bytes) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_budget_bytes = bytes;
        evict_until_within_budget();
    }

    void put(K key, std::unique_ptr<T> item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::size_t cost = m_cost_func(*item);
        
        // Remove existing if any
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            m_current_bytes -= m_cost_func(*it->second.first);
            m_list.erase(it->second.second);
            m_map.erase(it);
        }

        m_list.push_front(key);
        m_map[key] = { std::move(item), m_list.begin() };
        m_current_bytes += cost;

        evict_until_within_budget();
    }

    const T* get(const K& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            return nullptr;
        }

        // Move to front (MRU)
        m_list.splice(m_list.begin(), m_list, it->second.second);
        return it->second.first.get();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_map.clear();
        m_list.clear();
        m_current_bytes = 0;
    }

    std::size_t current_usage() const {
        return m_current_bytes;
    }

private:
    void evict_until_within_budget() {
        while (m_current_bytes > m_budget_bytes && !m_list.empty()) {
            K oldest_key = m_list.back();
            auto it = m_map.find(oldest_key);
            if (it != m_map.end()) {
                m_current_bytes -= m_cost_func(*it->second.first);
                m_map.erase(it);
            }
            m_list.pop_back();
        }
    }

    std::size_t m_budget_bytes;
    std::size_t m_current_bytes{0};
    CostFunc m_cost_func;

    std::list<K> m_list;
    std::unordered_map<K, std::pair<std::unique_ptr<T>, typename std::list<K>::iterator>> m_map;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::media
