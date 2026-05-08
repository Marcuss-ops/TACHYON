#include "tachyon/presets/sfx/sfx_registry.h"

#include <utility>
#include <algorithm>
#include <random>

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

std::vector<std::string> SfxRegistry::list_sound_effects(
    const media::AssetResolver& resolver,
    SfxCategory category) const {
    std::vector<std::string> files;
    const auto* info = get_info(category);
    if (!info) return files;

    auto sfx_root = resolver.config().sfx_root;
    if (sfx_root.empty()) return files;

    auto dir = std::filesystem::path(sfx_root) / info->folder;
    if (!std::filesystem::exists(dir)) return files;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".m4a" || ext == ".mp3" || ext == ".wav") {
                files.push_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::optional<spec::AudioTrackSpec> SfxRegistry::create_random_sound_track(
    const media::AssetResolver& resolver,
    SfxCategory category,
    std::uint64_t seed,
    float volume) const {
    auto files = list_sound_effects(resolver, category);
    if (files.empty()) return std::nullopt;

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
    auto path = files[dist(rng)];

    spec::AudioTrackSpec spec;
    spec.source_path = path;
    spec.volume = volume;
    return spec;
}

} // namespace tachyon::presets
