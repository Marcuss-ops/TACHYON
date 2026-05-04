#include "tachyon/presets/sfx/sfx_registry.h"

namespace tachyon::presets {

SfxRegistry& SfxRegistry::instance() {
    static SfxRegistry instance;
    return instance;
}

SfxRegistry::SfxRegistry() {
    // Register default categories
    register_category({SfxCategory::TypeWriting, "typewriting", "TypeWriting", ".m4a", true});
    register_category({SfxCategory::Mouse, "mouse", "Mouse", ".m4a", true});
    register_category({SfxCategory::Photo, "photo", "Photo", ".m4a", true});
    register_category({SfxCategory::Soosh, "soosh", "Soosh", ".m4a", true});
    register_category({SfxCategory::MoneySound, "money", "MoneySound", ".m4a", true});
}

void SfxRegistry::register_category(SfxCategoryInfo info) {
    m_categories[info.category] = std::move(info);
}

const SfxCategoryInfo* SfxRegistry::get_info(SfxCategory category) const {
    auto it = m_categories.find(category);
    if (it != m_categories.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string SfxRegistry::get_folder(SfxCategory category) const {
    if (auto* info = get_info(category)) {
        return info->folder;
    }
    return "";
}

} // namespace tachyon::presets
