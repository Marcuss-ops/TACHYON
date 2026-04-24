#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace tachyon {
namespace media {

enum class AssetType {
    VIDEO,
    AUDIO,
    IMAGE,
    MESH,
    FONT,
    PRESET,
    PROJECT
};

struct AssetMetadata {
    std::string id;
    std::string name;
    std::string path;
    std::string proxy_path;
    AssetType type;
    std::chrono::system_clock::time_point import_time;
    uint64_t file_size = 0;
    uint64_t duration_ms = 0; // for video/audio
    int width = 0, height = 0; // for video/image
    int sample_rate = 0, channels = 0; // for audio
    std::string codec;
    std::string resolution;
    bool has_proxy = false;
    bool is_hd = false;
    std::unordered_map<std::string, std::string> custom_properties;
};

class AssetManager {
public:
    AssetManager();
    ~AssetManager();
    
    // Asset registration
    std::string register_asset(const std::string& file_path, AssetType type);
    bool unregister_asset(const std::string& asset_id);
    bool update_asset_metadata(const std::string& asset_id, const AssetMetadata& metadata);
    
    // Asset retrieval
    std::optional<AssetMetadata> get_asset(const std::string& asset_id) const;
    std::vector<AssetMetadata> get_assets_by_type(AssetType type) const;
    std::vector<AssetMetadata> search_assets(const std::string& query) const;
    
    // Proxy management
    bool generate_proxy(const std::string& asset_id, int max_width = 1920);
    bool delete_proxy(const std::string& asset_id);
    std::string get_proxy_path(const std::string& asset_id) const;
    bool has_proxy(const std::string& asset_id) const;
    
    // Cache management
    void clear_cache();
    uint64_t get_cache_size() const;
    void set_cache_directory(const std::string& path);
    
    // Batch operations
    std::vector<std::string> batch_import(const std::vector<std::string>& file_paths);
    void batch_generate_proxies(const std::vector<std::string>& asset_ids);
    
    // Persistence
    bool save_asset_database(const std::string& db_path) const;
    bool load_asset_database(const std::string& db_path);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Proxy generation utilities
class ProxyGenerator {
public:
    struct ProxySettings {
        int max_resolution = 1920; // max width for proxy
        int video_bitrate_kbps = 2000;
        std::string video_codec = "libx264";
        int audio_bitrate_kbps = 128;
        std::string audio_codec = "aac";
        bool include_audio = true;
        float speed_factor = 1.0f; // playback speed of proxy
    };
    
    static bool generate_video_proxy(const std::string& input_path, 
                                   const std::string& output_path,
                                   const ProxySettings& settings = {});
    
    static bool generate_image_proxy(const std::string& input_path,
                                   const std::string& output_path,
                                   int max_width = 1920);
    
    static std::string get_proxy_filename(const std::string& original_path);
};

// Cache manager for decoded frames/processed assets
class CacheManager {
public:
    struct CacheStats {
        uint64_t total_size_bytes = 0;
        uint64_t max_size_bytes = 10ULL * 1024 * 1024 * 1024; // 10GB default
        int num_entries = 0;
        std::string cache_directory;
    };
    
    explicit CacheManager(const std::string& cache_dir);
    ~CacheManager();
    
    // Frame cache
    bool cache_frame(const std::string& key, const uint8_t* data, size_t size);
    bool get_cached_frame(const std::string& key, uint8_t* data, size_t& size);
    bool has_cached_frame(const std::string& key) const;
    
    // Asset cache
    bool cache_asset(const std::string& key, const std::vector<uint8_t>& data);
    std::optional<std::vector<uint8_t>> get_cached_asset(const std::string& key);
    
    // Management
    void evict_oldest(int num_to_evict);
    void clear();
    CacheStats get_stats() const;
    void set_max_size(uint64_t max_bytes) { stats_.max_size_bytes = max_bytes; }
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    CacheStats stats_;
    std::string cache_dir_;
};

} // namespace media
} // namespace tachyon
