#include "tachyon/runtime/cache/disk_cache.h"
#include "tachyon/core/hash/hash.h"
#include "tachyon/core/compression/compress.h"
#include <spdlog/fmt/fmt.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace tachyon::runtime {

std::string CacheKey::to_filename() const {
    // Hash all parameters except node_path so the path stays readable for
    // invalidation (DiskCacheStore::invalidate searches by node_path prefix).
    tachyon::hash::Hasher64 h;
    h.update(identity)
     .update(frame_time)
     .update(quality_tier)
     .update(aov_type)
     .update(static_cast<int>(time_remap_mode))
     .update(static_cast<int>(frame_blend_mode))
     .update(frame_blend_strength)
     .update(shutter_angle_deg)
     .update(motion_blur_samples)
     .update(motion_blur_seed)
     .update(static_cast<uint8_t>(use_proxy))
     .update(proxy_variant_id);

    std::string safe_path = node_path;
    std::replace_if(safe_path.begin(), safe_path.end(), [](char c) {
        return !std::isalnum(c) && c != '_';
    }, '_');

    if (safe_path.empty()) {
        return fmt::format("frame_{:016x}.bin", h.digest());
    }
    return fmt::format("frame_{}_{:016x}.bin", safe_path, h.digest());
}

CacheKey CacheKey::build(
    std::uint64_t identity,
    const std::string& node_path,
    double frame_time,
    int quality_tier,
    const std::string& aov_type,
    const spec::TimeRemapMode time_remap,
    const spec::FrameBlendMode blend_mode,
    float blend_strength,
    float shutter_angle,
    int mb_samples,
    std::uint32_t mb_seed,
    bool proxy,
    const std::string& proxy_id
) {
    CacheKey key;
    key.identity = identity;
    key.node_path = node_path;
    key.frame_time = frame_time;
    key.quality_tier = quality_tier;
    key.aov_type = aov_type;
    key.time_remap_mode = time_remap;
    key.frame_blend_mode = blend_mode;
    key.frame_blend_strength = blend_strength;
    key.shutter_angle_deg = shutter_angle;
    key.motion_blur_samples = mb_samples;
    key.motion_blur_seed = mb_seed;
    key.use_proxy = proxy;
    key.proxy_variant_id = proxy_id;
    return key;
}

DiskCacheStore::DiskCacheStore(std::filesystem::path root_dir)
    : m_root(std::move(root_dir))
{
    fs::create_directories(m_root);
}

void DiskCacheStore::store(const CacheKey& key, const std::vector<uint8_t>& frame_data) {
    auto path = m_root / key.to_filename();
    fs::create_directories(path.parent_path());

    auto compressed = tachyon::compression::compress(frame_data);
    if (compressed.empty()) return;

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (ofs) {
        ofs.write(reinterpret_cast<const char*>(compressed.data()),
                  static_cast<std::streamsize>(compressed.size()));
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.total_size_bytes += compressed.size();
        m_stats.entry_count++;
    }
}

std::optional<std::vector<uint8_t>> DiskCacheStore::load(const CacheKey& key) {
    auto path = m_root / key.to_filename();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!fs::exists(path)) {
            m_stats.misses++;
            return std::nullopt;
        }
    }

    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.misses++;
        return std::nullopt;
    }

    auto end = ifs.tellg();
    if (end <= 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.misses++;
        return std::nullopt;
    }
    ifs.seekg(0, std::ios::beg);

    std::vector<uint8_t> raw(static_cast<std::size_t>(end));
    if (!ifs.read(reinterpret_cast<char*>(raw.data()), end)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.misses++;
        return std::nullopt;
    }

    auto decompressed = tachyon::compression::try_decompress(raw);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!decompressed) {
        m_stats.misses++;
        return std::nullopt;
    }
    m_stats.hits++;
    return decompressed;
}

void DiskCacheStore::invalidate(const std::string& node_path_prefix) {
    std::vector<fs::path> to_delete;

    for (const auto& entry : fs::recursive_directory_iterator(m_root)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(node_path_prefix) != std::string::npos) {
                to_delete.push_back(entry.path());
            }
        }
    }

    for (const auto& path : to_delete) {
        auto size = fs::file_size(path);
        if (fs::remove(path)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stats.total_size_bytes = (m_stats.total_size_bytes >= size)
                                       ? m_stats.total_size_bytes - size : 0;
            if (m_stats.entry_count > 0) m_stats.entry_count--;
        }
    }
}

void DiskCacheStore::purge_to_budget(std::size_t max_bytes) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_stats.total_size_bytes <= max_bytes) return;
    std::size_t current_total = m_stats.total_size_bytes;
    lock.unlock();

    struct Entry {
        fs::path path;
        std::filesystem::file_time_type mtime;
        std::size_t size;
    };

    std::vector<Entry> entries;
    for (const auto& entry : fs::recursive_directory_iterator(m_root)) {
        if (entry.is_regular_file()) {
            entries.push_back({
                entry.path(),
                entry.last_write_time(),
                static_cast<std::size_t>(entry.file_size())
            });
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.mtime < b.mtime; });

    std::size_t freed = 0;
    std::size_t target = current_total > max_bytes ? current_total - max_bytes : 0;

    for (const auto& entry : entries) {
        if (freed >= target) break;
        if (fs::remove(entry.path)) {
            freed += entry.size;
            std::lock_guard<std::mutex> lock2(m_mutex);
            if (m_stats.entry_count > 0) m_stats.entry_count--;
        }
    }

    std::lock_guard<std::mutex> lock3(m_mutex);
    m_stats.total_size_bytes = (freed <= m_stats.total_size_bytes)
                               ? m_stats.total_size_bytes - freed : 0;
}

DiskCacheStore::Stats DiskCacheStore::get_stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

} // namespace tachyon::runtime
