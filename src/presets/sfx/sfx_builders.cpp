#include "tachyon/presets/sfx/sfx_builders.h"
#include "tachyon/presets/sfx/sfx_registry.h"
#include "tachyon/audio/core/sound_effect_registry.h"
#include <vector>
#include <algorithm>

namespace tachyon::presets {

namespace spec = tachyon::spec;
using spec::AudioTrackSpec;

AudioTrackSpec build_sfx(const media::AssetResolver& resolver, const SfxParams& p, DiagnosticBag* diagnostics) {
    AudioTrackSpec spec;
    
    auto& registry = SfxRegistry::instance();
    const auto* info = registry.get_info(p.category);
    
    if (!info) {
        if (diagnostics) {
            diagnostics->add_error("SFX_UNKNOWN_CATEGORY", "Unknown SFX category requested.");
        }
        return spec;
    }

    std::string category_name = info->folder;

    if (p.variant == -1) {
        // Deterministic random selection via the official registry
        auto track = audio::createRandomSoundTrack(resolver, category_name, p.seed, p.volume);
        if (track) {
            spec = *track;
        } else if (diagnostics) {
            diagnostics->add_error("SFX_RESOLVE_FAILED", "Failed to resolve a random SFX for category: " + category_name);
        }
    } else {
        // Direct variant selection
        auto sfx_root = resolver.config().sfx_root;
        if (!sfx_root.empty()) {
            auto path = sfx_root / category_name / (std::to_string(p.variant) + info->extension);
            if (std::filesystem::exists(path)) {
                spec.source_path = path.string();
            } else if (diagnostics) {
                diagnostics->add_error("SFX_VARIANT_NOT_FOUND", "SFX variant " + std::to_string(p.variant) + " not found in category " + category_name);
            }
        } else if (diagnostics) {
            diagnostics->add_error("SFX_ROOT_NOT_SET", "SFX root directory is not configured in AssetResolver.");
        }
    }

    spec.in_point_seconds = static_cast<double>(p.in_point);
    spec.out_point_seconds = static_cast<double>(p.out_point);
    spec.volume = p.volume;
    return spec;
}

AudioTrackSpec build_sfx(const media::AssetResolver& resolver, SfxCategory cat, double trigger_time, float volume, DiagnosticBag* diagnostics) {
    SfxParams p;
    p.category = cat;
    p.in_point = trigger_time;
    p.volume = volume;
    p.seed = 0; // Default seed
    return build_sfx(resolver, p, diagnostics);
}

} // namespace tachyon::presets
