#include "tachyon/presets/sfx/sfx_registry.h"

#include <utility>
#include <algorithm>
#include <random>

namespace tachyon::presets {

struct SfxAssetDef {
    const char* relative_path;
    SfxCategory category;
};

static constexpr SfxAssetDef kSfxAssets[] = {
    {"MoneySound/0.m4a", SfxCategory::MoneySound},
    {"MoneySound/Best SFX Sound Effects Catalog to Download Artlist-23.m4a", SfxCategory::MoneySound},
    {"MoneySound/Best SFX Sound Effects Catalog to Download Artlist-24.m4a", SfxCategory::MoneySound},
    {"MoneySound/Best SFX Sound Effects Catalog to Download Artlist-25.m4a", SfxCategory::MoneySound},
    {"Mouse/0.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-26.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-27.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-32.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-33.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-34.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-35.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-36.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-37.m4a", SfxCategory::Mouse},
    {"Mouse/Best SFX Sound Effects Catalog to Download Artlist-38.m4a", SfxCategory::Mouse},
    {"Photo/0.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-08.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-09.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-10.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-11.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-12.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-13.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-14.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-15.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-16.m4a", SfxCategory::Photo},
    {"Photo/Best SFX Sound Effects Catalog to Download Artlist-17.m4a", SfxCategory::Photo},
    {"Soosh/0.m4a", SfxCategory::Soosh},
    {"Soosh/Artlist Studios - Sound Effects Albums Artlist.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-19.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-20.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-21.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-22.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-28.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-29.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist-31.m4a", SfxCategory::Soosh},
    {"Soosh/Best SFX Sound Effects Catalog to Download Artlist.m4a", SfxCategory::Soosh},
    {"TypeWriting/0.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/1.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/2.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-01.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-02.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-03.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-04.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-05.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-06.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-07.m4a", SfxCategory::TypeWriting},
    {"TypeWriting/Best SFX Sound Effects Catalog to Download Artlist-18.m4a", SfxCategory::TypeWriting}
};

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
    auto sfx_root = resolver.config().sfx_root;
    if (sfx_root.empty()) return files;
    
    std::filesystem::path root_path(sfx_root);
    
    for (const auto& asset : kSfxAssets) {
        if (asset.category == category) {
            files.push_back((root_path / asset.relative_path).string());
        }
    }
    
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