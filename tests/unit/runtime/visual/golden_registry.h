#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace tachyon::test {

/**
 * @brief Simple registry for "Golden" hashes to ensure bit-identity across builds.
 */
class GoldenRegistry {
public:
    explicit GoldenRegistry(std::string path) : m_path(std::move(path)) {
        load();
    }

    [[nodiscard]] std::uint64_t get_hash(const std::string& key) const {
        auto it = m_hashes.find(key);
        if (it != m_hashes.end()) {
            return it->second;
        }
        return 0;
    }

    void set_hash(const std::string& key, std::uint64_t hash) {
        m_hashes[key] = hash;
    }

    void load() {
        std::ifstream f(m_path);
        if (!f.is_open()) return;
        try {
            nlohmann::json j;
            f >> j;
            for (auto& [k, v] : j.items()) {
                m_hashes[k] = v.get<std::uint64_t>();
            }
        } catch (...) {}
    }

    void save() {
        nlohmann::json j;
        for (auto& [k, v] : m_hashes) {
            j[k] = v;
        }
        std::ofstream f(m_path);
        f << j.dump(2);
    }

    bool verify_or_update(const std::string& key, std::uint64_t current_hash, bool update_mode = false) {
        std::uint64_t expected = get_hash(key);
        if (update_mode) {
            set_hash(key, current_hash);
            save();
            return true;
        }
        return expected == current_hash;
    }

private:
    std::string m_path;
    std::map<std::string, std::uint64_t> m_hashes;
};

} // namespace tachyon::test
