#include "tachyon/presets/sfx/sfx_builders.h"
#include "tachyon/audio/core/sound_effect_registry.h"
#include <map>
#include <vector>
#include <algorithm>

namespace tachyon::presets {

namespace spec = tachyon::spec;
using spec::AudioTrackSpec;

namespace {

std::string category_to_string(SfxCategory category) {
    static const std::map<SfxCategory, std::string> category_folders = {
        {SfxCategory::TypeWriting, "TypeWriting"},
        {SfxCategory::Mouse, "Mouse"},
        {SfxCategory::Photo, "Photo"},
        {SfxCategory::Soosh, "Soosh"},
        {SfxCategory::MoneySound, "MoneySound"},
    };
    auto it = category_folders.find(category);
    if (it == category_folders.end()) {
        return "";
    }
    return it->second;
}

} // anonymous namespace

AudioTrackSpec build_sfx(const media::AssetResolver& resolver, const SfxParams& p) {
    AudioTrackSpec spec;
    
    std::string category_name = category_to_string(p.category);
    if (category_name.empty()) {
        return spec;
    }

    if (p.variant == -1) {
        // Deterministic random selection via the official registry
        auto track = audio::createRandomSoundTrack(resolver, category_name, p.seed, p.volume);
        if (track) {
            spec = *track;
        }
    } else {
        // Direct variant selection
        auto sfx_root = resolver.config().sfx_root;
        if (!sfx_root.empty()) {
            spec.source_path = (sfx_root / category_name / (std::to_string(p.variant) + ".m4a")).string();
        }
    }

    spec.in_point_seconds = static_cast<double>(p.in_point);
    spec.out_point_seconds = static_cast<double>(p.out_point);
    spec.volume = p.volume;
    return spec;
}

AudioTrackSpec build_sfx(const media::AssetResolver& resolver, SfxCategory cat, double trigger_time, float volume) {
    SfxParams p;
    p.category = cat;
    p.in_point = trigger_time;
    p.volume = volume;
    p.seed = 0; // Default seed
    return build_sfx(resolver, p);
}

} // namespace tachyon::presets
