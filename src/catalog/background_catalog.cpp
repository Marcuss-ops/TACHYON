#include "tachyon/background_catalog.h"

#include <utility>

namespace tachyon {

struct BackgroundCatalog::Impl {
    std::vector<BackgroundCatalogEntry> entries;
    std::map<std::string, std::size_t> id_to_index;
};

BackgroundCatalog& BackgroundCatalog::instance() {
    static BackgroundCatalog instance;
    return instance;
}

BackgroundCatalog::BackgroundCatalog() {
    m_impl = std::make_unique<Impl>();
    // Register built-in backgrounds
    register_entry({
        "tachyon.background.solid",
        "solid",
        R"({"color": "#000000"})",
        "",
        BackgroundStatus::Stable,
        "Solid color background"
    });

    register_entry({
        "tachyon.background.gradient",
        "gradient",
        R"({"color_start": "#000000", "color_end": "#ffffff", "angle": 0})",
        "",
        BackgroundStatus::Stable,
        "Gradient background"
    });

    register_entry({
        "tachyon.background.image",
        "image",
        R"({"path": ""})",
        "",
        BackgroundStatus::Stable,
        "Image background"
    });

    register_entry({
        "tachyon.background.cinematic_dark",
        "gradient",
        R"({"color_start": "#0a0a0a", "color_end": "#1a1a2e", "angle": 135})",
        "",
        BackgroundStatus::Stable,
        "Dark cinematic gradient background"
    });

    register_entry({
        "tachyon.background.youtube_news_blue",
        "gradient",
        R"({"color_start": "#0d47a1", "color_end": "#1976d2", "angle": 180})",
        "",
        BackgroundStatus::Stable,
        "YouTube news style blue gradient"
    });

    register_entry({
        "tachyon.background.soft_gradient",
        "gradient",
        R"({"color_start": "#f5f5f5", "color_end": "#e0e0e0", "angle": 45})",
        "",
        BackgroundStatus::Stable,
        "Soft light gray gradient"
    });

    register_entry({
        "tachyon.background.product_glow",
        "gradient",
        R"({"color_start": "#1a1a1a", "color_end": "#2d2d2d", "angle": 160})",
        "",
        BackgroundStatus::Stable,
        "Product showcase dark background with subtle glow effect"
    });

    register_entry({
        "tachyon.background.warm_brown",
        "solid",
        R"({"color": "#3e2723"})",
        "",
        BackgroundStatus::Stable,
        "Warm brown solid background for cozy content"
    });
}

BackgroundCatalog::~BackgroundCatalog() = default;

void BackgroundCatalog::register_entry(const BackgroundCatalogEntry& entry) {
    if (m_impl->id_to_index.count(entry.id)) {
        return;
    }

    std::size_t index = m_impl->entries.size();
    m_impl->entries.push_back(entry);
    m_impl->id_to_index[entry.id] = index;
}

void BackgroundCatalog::unregister_entry(std::string_view id) {
    auto it = m_impl->id_to_index.find(std::string(id));
    if (it == m_impl->id_to_index.end()) return;

    std::size_t index = it->second;
    m_impl->entries.erase(m_impl->entries.begin() + index);
    m_impl->id_to_index.erase(std::string(id));

    // Rebuild index
    m_impl->id_to_index.clear();
    for (std::size_t i = 0; i < m_impl->entries.size(); ++i) {
        m_impl->id_to_index[m_impl->entries[i].id] = i;
    }
}

const BackgroundCatalogEntry* BackgroundCatalog::find(std::string_view id) const {
    auto it = m_impl->id_to_index.find(std::string(id));
    if (it == m_impl->id_to_index.end()) return nullptr;
    return &m_impl->entries[it->second];
}

std::size_t BackgroundCatalog::count() const {
    return m_impl->entries.size();
}

const BackgroundCatalogEntry* BackgroundCatalog::get_by_index(std::size_t index) const {
    if (index >= m_impl->entries.size()) return nullptr;
    return &m_impl->entries[index];
}

std::vector<std::string> BackgroundCatalog::list_all_ids() const {
    std::vector<std::string> ids;
    ids.reserve(m_impl->entries.size());
    for (const auto& entry : m_impl->entries) {
        ids.push_back(entry.id);
    }
    return ids;
}

std::vector<BackgroundCatalogEntry> BackgroundCatalog::list_all() const {
    return m_impl->entries;
}

bool BackgroundCatalog::validate_preset(std::string_view preset_id, std::string& error) const {
    if (find(preset_id)) return true;

    error = "Background preset ID '" + std::string(preset_id) + "' not found in catalog";
    return false;
}

} // namespace tachyon
