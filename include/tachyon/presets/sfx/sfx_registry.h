#pragma once

#include "tachyon/presets/sfx/sfx_params.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon::presets {

/**
 * @brief Metadata for a sound effect category.
 */
struct SfxCategoryInfo {
    SfxCategory category;
    std::string id;             ///< internal identifier
    std::string folder;         ///< folder name in asset root
    std::string extension;      ///< e.g., ".m4a"
    bool supports_variants;
};

/**
 * @brief Central registry for SFX categories and metadata.
 */
class SfxRegistry {
public:
    static SfxRegistry& instance();

    const SfxCategoryInfo* get_info(SfxCategory category) const;
    std::string get_folder(SfxCategory category) const;
    
    void register_category(SfxCategoryInfo info);

private:
    SfxRegistry();
    std::unordered_map<SfxCategory, SfxCategoryInfo> m_categories;
};

} // namespace tachyon::presets
