#include "tachyon/core/presets/sfx/sfx_builders.h"
#include "tachyon/core/presets/sfx/sfx_registry.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <vector>
#include <algorithm>
#include <filesystem>

namespace tachyon::core::presets::sfx {

spec::AudioTrackSpec build_sfx(const ::tachyon::media::IAssetResolver& resolver, const SfxParams& p, ::tachyon::DiagnosticBag* diagnostics) {
    spec::AudioTrackSpec spec;
    
    auto& registry = SfxRegistry::instance();
    const auto* info = registry.get_info(p.category);
    
    if (!info) {
        if (diagnostics) {
            diagnostics->add_error("SFX_UNKNOWN_CATEGORY", "Unknown SFX category requested.");
        }
        return spec;
    }

    if (p.variant == -1) {
        // Deterministic random selection via the official registry
        auto track = registry.create_random_sound_track(resolver, p.category, p.seed, p.volume);
        if (track) {
            spec = *track;
        } else if (diagnostics) {
            diagnostics->add_error("SFX_RESOLVE_FAILED", "Failed to resolve a random SFX for category: " + info->folder);
        }
    } else {
        // Direct variant selection
        auto sfx_root = resolver.config().sfx_root;
        if (!sfx_root.empty()) {
            auto path = sfx_root / info->folder / (std::to_string(p.variant) + info->extension);
            if (std::filesystem::exists(path)) {
                spec.source_path = path.string();
            } else if (diagnostics) {
                diagnostics->add_error("SFX_VARIANT_NOT_FOUND", "SFX variant " + std::to_string(p.variant) + " not found in category " + info->folder);
            }
        } else if (diagnostics) {
            diagnostics->add_error("SFX_ROOT_NOT_SET", "SFX root directory is not configured in AssetResolver.");
        }
    }

    spec.volume = p.volume;
    spec.in_point_seconds = p.timing.source_in;
    spec.out_point_seconds = p.timing.source_out;
    return spec;
}

spec::AudioTrackSpec build_sfx(const ::tachyon::media::IAssetResolver& resolver, SfxCategory cat, double trigger_time, float volume, ::tachyon::DiagnosticBag* diagnostics) {
    SfxParams p;
    p.category = cat;
    p.volume = volume;
    p.seed = 0; // Default seed
    
    auto spec = build_sfx(resolver, p, diagnostics);
    spec.start_offset_seconds = trigger_time;
    return spec;
}

} // namespace tachyon::core::presets::sfx
