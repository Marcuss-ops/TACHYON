#pragma once

#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/presets/sfx/sfx_params.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
namespace tachyon::media { class AssetResolver; }
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <filesystem>

namespace tachyon::presets {

/**
 * @brief Metadata for a sound effect category.
 */
struct SfxCategoryInfo {
    std::string id;             ///< namespaced canonical identifier
    SfxCategory category;
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
    [[nodiscard]] std::vector<std::string> list_ids() const;
    void load_builtins();

    std::optional<spec::AudioTrackSpec> create_random_sound_track(
        const media::AssetResolver& resolver,
        SfxCategory category,
        std::uint64_t seed,
        float volume = 1.0f) const;

    std::vector<std::string> list_sound_effects(
        const media::AssetResolver& resolver,
        SfxCategory category) const;

private:
    SfxRegistry();
    ~SfxRegistry();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::presets
