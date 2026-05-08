#include "tachyon/background_registry.h"
#include "tachyon/backgrounds.hpp"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/background/background_params.h"
#include <algorithm>

namespace tachyon {

struct BackgroundRegistry::Impl {
    std::unordered_map<std::string, BackgroundDescriptor> descriptors;
    std::unordered_map<std::string, std::string> alias_to_id;
};

BackgroundRegistry::BackgroundRegistry() : m_impl(std::make_unique<Impl>()) {
    register_all_builtins();
}

BackgroundRegistry::~BackgroundRegistry() = default;

void BackgroundRegistry::register_all_builtins() {
    // Basic built-in backgrounds
    BackgroundDescriptor solid;
    solid.id = "tachyon.background.solid";
    solid.display_name = "Solid Color";
    solid.kind = BackgroundKind::Solid;
    register_descriptor(solid);

    BackgroundDescriptor image;
    image.id = "tachyon.background.image";
    image.display_name = "Image";
    image.kind = BackgroundKind::Image;
    register_descriptor(image);
}

BackgroundRegistry& BackgroundRegistry::instance() {
    static BackgroundRegistry instance;
    return instance;
}

void BackgroundRegistry::register_descriptor(const BackgroundDescriptor& descriptor) {
    // Register aliases
    for (const auto& alias : descriptor.aliases) {
        m_impl->alias_to_id[alias] = descriptor.id;
    }

    // Register the descriptor
    m_impl->descriptors[descriptor.id] = descriptor;
}

const BackgroundDescriptor* BackgroundRegistry::find_by_id(std::string_view id) const {
    auto it = m_impl->descriptors.find(std::string(id));
    if (it != m_impl->descriptors.end()) {
        return &it->second;
    }
    return nullptr;
}

const BackgroundDescriptor* BackgroundRegistry::find_by_alias(std::string_view alias) const {
    auto it = m_impl->alias_to_id.find(std::string(alias));
    if (it != m_impl->alias_to_id.end()) {
        return find_by_id(it->second);
    }
    return nullptr;
}

const BackgroundDescriptor* BackgroundRegistry::resolve(std::string_view id_or_alias) const {
    // Try ID first
    auto* desc = find_by_id(id_or_alias);
    if (desc) return desc;

    // Try alias
    return find_by_alias(id_or_alias);
}

std::vector<std::string> BackgroundRegistry::list_all_ids() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : m_impl->descriptors) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<const BackgroundDescriptor*> BackgroundRegistry::list_all() const {
    std::vector<const BackgroundDescriptor*> result;
    for (const auto& [id, desc] : m_impl->descriptors) {
        result.push_back(&desc);
    }
    return result;
}

void BackgroundRegistry::unregister_background(std::string_view id) {
    auto it = m_impl->descriptors.find(std::string(id));
    if (it == m_impl->descriptors.end()) return;

    // Collect aliases to remove (cannot erase while iterating the same map)
    std::vector<std::string> aliases_to_remove;
    for (const auto& [alias, mapped_id] : m_impl->alias_to_id) {
        if (mapped_id == id) {
            aliases_to_remove.push_back(alias);
        }
    }

    // Now remove aliases
    for (const auto& alias : aliases_to_remove) {
        m_impl->alias_to_id.erase(alias);
    }

    m_impl->descriptors.erase(it);
}

} // namespace tachyon
