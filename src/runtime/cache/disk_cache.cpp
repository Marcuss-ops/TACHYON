#include "tachyon/runtime/cache/disk_cache.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace tachyon::runtime {

std::string CacheKey::to_filename() const {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << identity;
    oss << "_" << (node_path.empty() ? "root" : node_path);
    oss << "_" << std::fixed << std::setprecision(4) << frame_time;
    oss << "_q" << quality_tier;
    oss << "_" << aov_type << ".bin";
    
    std::string s = oss.str();
    // Sanitize node path for filesystem
    std::replace(s.begin(), s.end(), '/', '_');
    std::replace(s.begin(), s.end(), ':', '_');
    return s;
}

DiskCacheStore::DiskCacheStore(std::filesystem::path root_dir) 
    : m_root(std::move(root_dir)) {
    if (!std::filesystem::exists(m_root)) {
        std::filesystem::create_directories(m_root);
    }
}

void DiskCacheStore::store(const CacheKey& key, const std::vector<uint8_t>& frame_data) {
    const auto path = m_root / key.to_filename();
    std::ofstream file(path, std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(frame_data.data()), frame_data.size());
    }
}

std::optional<std::vector<uint8_t>> DiskCacheStore::load(const CacheKey& key) {
    const auto path = m_root / key.to_filename();
    if (!std::filesystem::exists(path)) {
        m_stats.misses++;
        return std::nullopt;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        m_stats.misses++;
        return std::nullopt;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        m_stats.hits++;
        return buffer;
    }

    m_stats.misses++;
    return std::nullopt;
}

void DiskCacheStore::invalidate(const std::string& node_path_prefix) {
    if (node_path_prefix.empty()) {
        std::filesystem::remove_all(m_root);
        std::filesystem::create_directories(m_root);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_root)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            // Filename format: hash_nodepath_time_qX_aov.bin
            // We search for the node_path part
            if (filename.find("_" + node_path_prefix) != std::string::npos) {
                std::filesystem::remove(entry.path());
            }
        }
    }
}

DiskCacheStore::Stats DiskCacheStore::get_stats() const {
    std::size_t total_size = 0;
    std::size_t count = 0;
    if (std::filesystem::exists(m_root)) {
        for (const auto& entry : std::filesystem::directory_iterator(m_root)) {
            if (entry.is_regular_file()) {
                total_size += std::filesystem::file_size(entry);
                count++;
            }
        }
    }
    m_stats.total_size_bytes = total_size;
    m_stats.entry_count = count;
    return m_stats;
}

void DiskCacheStore::purge_to_budget(std::size_t max_bytes) {
    auto stats = get_stats();
    if (stats.total_size_bytes <= max_bytes) return;

    // Industrial eviction: sort by access time (or just modification time as proxy)
    struct Entry {
        std::filesystem::path path;
        std::filesystem::file_time_type time;
        std::size_t size;
    };
    std::vector<Entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(m_root)) {
        if (entry.is_regular_file()) {
            entries.push_back({entry.path(), entry.last_write_time(), std::filesystem::file_size(entry)});
        }
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.time < b.time;
    });

    std::size_t current_size = stats.total_size_bytes;
    for (const auto& entry : entries) {
        if (current_size <= max_bytes) break;
        if (std::filesystem::remove(entry.path)) {
            current_size -= entry.size;
        }
    }
}

} // namespace tachyon::runtime