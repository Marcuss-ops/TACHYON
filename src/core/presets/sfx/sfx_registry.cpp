#include "tachyon/core/presets/sfx/sfx_registry.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/core/registry/typed_registry.h"

#include <utility>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <filesystem>

namespace tachyon::core::presets::sfx {

namespace spec = tachyon::spec;

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
    static SfxRegistry instance_val;
    return instance_val;
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
    const ::tachyon::media::IAssetResolver& resolver,
    SfxCategory category) const {
    
    std::vector<std::string> results;
    auto sfx_root_path = resolver.config().sfx_root;
    if (sfx_root_path.empty()) return results;
    
    std::filesystem::path root_fs_path(sfx_root_path);
    
    for (const auto& asset_def : kSfxAssets) {
        if (asset_def.category == category) {
            results.push_back((root_fs_path / asset_def.relative_path).string());
        }
    }
    
    return results;
}

std::optional<spec::AudioTrackSpec> SfxRegistry::create_random_sound_track(
    const ::tachyon::media::IAssetResolver& resolver,
    SfxCategory category,
    std::uint64_t seed,
    float volume_param) const {
    
    std::vector<std::string> sound_files = list_sound_effects(resolver, category);
    if (sound_files.empty()) return std::nullopt;

    std::mt19937_64 random_gen(seed);
    std::uniform_int_distribution<size_t> dist_indices(0, sound_files.size() - 1);
    std::string selected_path = sound_files.at(dist_indices(random_gen));

    spec::AudioTrackSpec result_spec;
    result_spec.source_path = selected_path;
    result_spec.volume = volume_param;
    return result_spec;
}

} // namespace tachyon::core::presets::sfx
