#pragma once

#include "tachyon/core/media/media_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>

namespace tachyon::media {

/**
 * @brief Technical metadata for an asset.
 */
struct AssetMetadata {
    std::string id;
    std::string uri;
    AssetType type{AssetType::UNKNOWN};
    uint64_t size_bytes{0};
    double duration{0.0};
    std::string metadata_json;
};

/**
 * @brief Manages asset metadata and identity.
 */
class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    /**
     * @brief Registers an asset in the manager.
     * @return The unique ID assigned to the asset.
     */
    std::string register_asset(const std::string& file_path, AssetType type = AssetType::UNKNOWN);
    
    /**
     * @brief Unregisters an asset.
     */
    bool unregister_asset(const std::string& asset_id);

    /**
     * @brief Updates metadata for an existing asset.
     */
    bool update_asset_metadata(const std::string& asset_id, const AssetMetadata& metadata);

    /**
     * @brief Gets metadata for an asset.
     */
    std::optional<AssetMetadata> get_asset(const std::string& asset_id) const;

    /**
     * @brief Gets all assets of a specific type.
     */
    std::vector<AssetMetadata> get_assets_by_type(AssetType type) const;

    /**
     * @brief Searches for assets by name or ID.
     */
    std::vector<AssetMetadata> search_assets(const std::string& query) const;

    /**
     * @brief Triggers proxy generation for an asset.
     */
    bool generate_proxy(const std::string& asset_id, int max_width = 1280);
    
    /**
     * @brief Deletes proxy for an asset.
     */
    bool delete_proxy(const std::string& asset_id);

    /**
     * @brief Gets proxy path for an asset if it exists.
     */
    std::string get_proxy_path(const std::string& asset_id) const;

    /**
     * @brief Checks if an asset has a proxy available.
     */
    bool has_proxy(const std::string& asset_id) const;

    /**
     * @brief Clears volatile caches.
     */
    void clear_cache();

    /**
     * @brief Gets total cache size in bytes.
     */
    uint64_t get_cache_size() const;

    /**
     * @brief Sets directory for proxy storage.
     */
    void set_cache_directory(const std::string& path);

    /**
     * @brief Batch import files.
     */
    std::vector<std::string> batch_import(const std::vector<std::string>& file_paths);

    /**
     * @brief Batch generate proxies.
     */
    void batch_generate_proxies(const std::vector<std::string>& asset_ids);

    /**
     * @brief Persistence.
     */
    bool save_asset_database(const std::string& db_path) const;
    bool load_asset_database(const std::string& db_path);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tachyon::media
