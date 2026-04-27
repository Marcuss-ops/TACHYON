#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace tachyon::test {

/**
 * @brief Simple registry for "Golden" hashes to ensure bit-identity across builds.
 */
class GoldenRegistry {
public:
    static void set_update_mode(bool enabled) { s_update_mode = enabled; }
    static bool get_update_mode() { return s_update_mode; }

    explicit GoldenRegistry(std::string path) {
        // Resolve path relative to test source dir if not absolute
        std::filesystem::path p(path);
        if (!p.is_absolute()) {
#ifdef TACHYON_TESTS_SOURCE_DIR
            p = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / path;
#else
            p = std::filesystem::current_path() / path;
#endif
        }
        m_path = p.string();
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
                if (v.is_string()) {
                    std::string s = v.get<std::string>();
                    // Remove 0x prefix if present
                    if (s.rfind("0x", 0) == 0) s.erase(0, 2);
                    m_hashes[k] = std::stoull(s, nullptr, 16);
                } else {
                    m_hashes[k] = v.get<std::uint64_t>();
                }
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

    bool verify_or_update(const std::string& key, std::uint64_t current_hash) {
        std::uint64_t expected = get_hash(key);
        if (s_update_mode) {
            set_hash(key, current_hash);
            save();
            return true;
        }
        return expected == current_hash;
    }

private:
    static inline bool s_update_mode = false;
    std::string m_path;
    std::map<std::string, std::uint64_t> m_hashes;
};

} // namespace tachyon::test
