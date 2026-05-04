#pragma once

#include "tachyon/presets/sfx/sfx_params.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/media/management/asset_resolver.h"

namespace tachyon::presets {

namespace spec = tachyon::spec;

[[nodiscard]] spec::AudioTrackSpec build_sfx(const media::AssetResolver& resolver, const SfxParams& p);
[[nodiscard]] spec::AudioTrackSpec build_sfx(const media::AssetResolver& resolver, SfxCategory cat, double trigger_time, float volume = 1.0f);

} // namespace tachyon::presets
