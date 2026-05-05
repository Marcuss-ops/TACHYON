#include "tachyon/presets/sfx/sfx_registry.h"

#include <utility>

namespace tachyon::presets {

struct SfxRegistry::Impl {
    registry::TypedRegistry<SfxCategoryInfo> categories;
    std::unordered_map<SfxCategory, std::string> category_ids;
};

SfxRegistry& SfxRegistry::instance() {
    static SfxRegistry instance;
    return instance;
}

SfxRegistry::SfxRegistry() : m_impl(std::make_unique<Impl>()) {
    load_builtins();
}

SfxRegistry::~SfxRegistry() = default;

void SfxRegistry::register_category(SfxCategoryInfo info) {
    if (info.id.empty()) {
        return;
    }

    m_impl->category_ids[info.category] = info.id;
    m_impl->categories.register_spec(std::move(info));
}

const SfxCategoryInfo* SfxRegistry::get_info(SfxCategory category) const {
    const auto it = m_impl->category_ids.find(category);
    if (it == m_impl->category_ids.end()) {
        return nullptr;
    }
    return m_impl->categories.find(it->second);
}

std::string SfxRegistry::get_folder(SfxCategory category) const {
    if (auto* info = get_info(category)) {
        return info->folder;
    }
    return "";
}

std::vector<std::string> SfxRegistry::list_ids() const {
    return m_impl->categories.list_ids();
}

} // namespace tachyon::presets
