#pragma once

#include "tachyon/presets/sfx/sfx_params.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
namespace tachyon { class DiagnosticBag; }
namespace tachyon::media { class AssetResolver; }

namespace tachyon::presets {

namespace spec = tachyon::spec;

[[nodiscard]] spec::AudioTrackSpec build_sfx(const media::AssetResolver& resolver, const SfxParams& p, ::tachyon::DiagnosticBag* diagnostics = nullptr);
[[nodiscard]] spec::AudioTrackSpec build_sfx(const media::AssetResolver& resolver, SfxCategory cat, double trigger_time, float volume = 1.0f, ::tachyon::DiagnosticBag* diagnostics = nullptr);

} // namespace tachyon::presets
