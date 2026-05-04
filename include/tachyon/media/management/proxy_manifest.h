#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon::media {

/**
 * @brief Profile for proxy selection.
 */
enum class ProxyProfile {
    Playback,  // Optimized for playback (lower resolution/bitrate)
    Export,    // High quality proxy for export
    Analysis   // Proxy for analysis operations
};

/**
 * @brief Represents a specific proxy file variant for a piece of source media.
 */
struct ProxyVariant {
    std::string original_path;
    std::string proxy_path;
    int width{0};
    int height{0};
    std::string codec; ///< "h264", "prores_proxy", "dnxhr_lb", etc.
    float bitrate_mbps{0.0f};
};

/**
 * @brief Maintains the mapping between original media and its proxy versions.
 * 
 * Rules:
 * - One source file can have multiple proxy variants (e.g. 540p H264, 1080p ProRes).
 * - The manifest should be persistent (serialized/deserialized with the project).
 * - Only provides proxy lookup; path resolution decisions are made by PathResolver.
 */
class ProxyManifest {
public:
    ProxyManifest() = default;

    /**
     * @brief Register a new proxy variant for an original file.
     */
    void register_proxy(const ProxyVariant& variant);

    /**
     * @brief Find a proxy for the given original path and profile.
     * @return Path to the proxy, or empty path if not found.
     */
    [[nodiscard]] std::string find_proxy(const std::string& original_path, 
                                        ProxyProfile profile = ProxyProfile::Playback) const;

    /**
     * @brief Resolve the best proxy path for a given playback target.
     * 
     * @param original Path to the original file.
     * @param target_width Requested width for playback (e.g. viewport width).
     * @return Path to the best proxy, or original path if no suitable proxy exists.
     */
    [[nodiscard]] std::string resolve_for_playback(const std::string& original, int target_width) const;

    /**
     * @brief Resolve the path for final rendering (always returns original).
     */
    [[nodiscard]] std::string resolve_for_render(const std::string& original) const;

    /**
     * @brief Clear all proxy mappings.
     */
    void clear();

private:
    std::unordered_multimap<std::string, ProxyVariant> m_variants;
};

} // namespace tachyon::media
