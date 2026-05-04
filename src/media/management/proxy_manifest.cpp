#include "tachyon/media/management/proxy_manifest.h"
#include <algorithm>
#include <limits>

namespace tachyon::media {

void ProxyManifest::register_proxy(const ProxyVariant& variant) {
    m_variants.emplace(variant.original_path, variant);
}

std::string ProxyManifest::find_proxy(const std::string& original_path, 
                                      ProxyProfile profile) const {
    auto range = m_variants.equal_range(original_path);
    if (range.first == range.second) {
        return "";
    }

    if (profile == ProxyProfile::Playback) {
        // Return first available proxy for playback
        return range.first->second.proxy_path;
    } else if (profile == ProxyProfile::Export) {
        // Return highest quality proxy for export
        const ProxyVariant* best = nullptr;
        int best_width = -1;
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.width > best_width) {
                best_width = it->second.width;
                best = &it->second;
            }
        }
        return best ? best->proxy_path : "";
    } else if (profile == ProxyProfile::Analysis) {
        // Return smallest proxy for analysis
        const ProxyVariant* best = nullptr;
        int best_width = std::numeric_limits<int>::max();
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.width < best_width) {
                best_width = it->second.width;
                best = &it->second;
            }
        }
        return best ? best->proxy_path : "";
    }

    return "";
}

std::string ProxyManifest::resolve_for_playback(const std::string& original, int target_width) const {
    auto range = m_variants.equal_range(original);
    if (range.first == range.second) {
        return original;
    }

    // Find the proxy with width closest to target_width but not smaller than target_width/2
    const ProxyVariant* best = nullptr;
    int best_diff = std::numeric_limits<int>::max();

    for (auto it = range.first; it != range.second; ++it) {
        const auto& variant = it->second;
        int diff = std::abs(variant.width - target_width);
        if (diff < best_diff) {
            best_diff = diff;
            best = &variant;
        }
    }

    return best ? best->proxy_path : original;
}

std::string ProxyManifest::resolve_for_render(const std::string& original) const {
    return original;
}

void ProxyManifest::clear() {
    m_variants.clear();
}

} // namespace tachyon::media
