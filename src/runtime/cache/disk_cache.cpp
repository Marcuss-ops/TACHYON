#include "tachyon/runtime/cache/disk_cache.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace tachyon::runtime {

std::string CacheKey::to_filename() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << "frame_" << std::setw(16) << identity;
    oss << "_t" << std::setw(8) << static_cast<std::uint64_t>(frame_time * 10000.0);
    oss << "_q" << quality_tier;
    
    // Include temporal effects in filename
    oss << "_tr" << static_cast<int>(time_remap_mode);
    oss << "_fb" << static_cast<int>(frame_blend_mode);
    if (frame_blend_mode != spec::FrameBlendMode::None) {
        oss << "s" << std::setw(3) << static_cast<int>(frame_blend_strength * 100.0f);
    }
    
    // Include motion blur params
    if (motion_blur_samples > 1) {
        oss << "_mb" << motion_blur_samples 
            << "a" << static_cast<int>(shutter_angle_deg)
            << "x" << motion_blur_seed;
    }
    
    // Include proxy info
    if (use_proxy && !proxy_variant_id.empty()) {
        oss << "_p" << proxy_variant_id;
    }
    
    if (!aov_type.empty() && aov_type != "beauty") {
        oss << "_" << aov_type;
    }
    
    oss << ".bin";
    return oss.str();
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
    // Ensure root directory exists
    fs::create_directories(m_root);
}

void DiskCacheStore::store(const CacheKey& key, const std::vector<uint8_t>& frame_data) {
    auto path = m_root / key.to_filename();
    
    // Create parent directories if needed
    fs::create_directories(path.parent_path());
    
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (ofs) {
        ofs.write(reinterpret_cast<const char*>(frame_data.data()), 
                  static_cast<std::streamsize>(frame_data.size()));
        m_stats.total_size_bytes += frame_data.size();
        m_stats.entry_count++;
    }
}

std::optional<std::vector<uint8_t>> DiskCacheStore::load(const CacheKey& key) {
    auto path = m_root / key.to_filename();
    
    if (!fs::exists(path)) {
        m_stats.misses++;
        return std::nullopt;
    }
    
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        m_stats.misses++;
        return std::nullopt;
    }
    
    auto size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (ifs.read(reinterpret_cast<char*>(buffer.data()), 
                 static_cast<std::streamsize>(size))) {
        m_stats.hits++;
        return buffer;
    }
    
    m_stats.misses++;
    return std::nullopt;
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
        fs::remove(path);
        if (m_stats.total_size_bytes >= size) {
            m_stats.total_size_bytes -= size;
        } else {
            m_stats.total_size_bytes = 0;
        }
        if (m_stats.entry_count > 0) {
            m_stats.entry_count--;
        }
    }
}

void DiskCacheStore::purge_to_budget(std::size_t max_bytes) {
    if (m_stats.total_size_bytes <= max_bytes) {
        return;
    }
    
    // Collect all entries with their modification times
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
    
    // Sort by modification time (oldest first)
    std::sort(entries.begin(), entries.end(), 
              [](const Entry& a, const Entry& b) {
                  return a.mtime < b.mtime;
              });
    
    // Delete oldest files until under budget
    std::size_t freed = 0;
    std::size_t target = m_stats.total_size_bytes - max_bytes;
    
    for (const auto& entry : entries) {
        if (freed >= target) break;
        
        fs::remove(entry.path);
        freed += entry.size;
        m_stats.entry_count--;
    }
    
    if (freed <= m_stats.total_size_bytes) {
        m_stats.total_size_bytes -= freed;
    } else {
        m_stats.total_size_bytes = 0;
    }
}

DiskCacheStore::Stats DiskCacheStore::get_stats() const {
    return m_stats;
}

} // namespace tachyon::runtime
