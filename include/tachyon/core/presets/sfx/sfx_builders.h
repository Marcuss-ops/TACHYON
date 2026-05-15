#pragma once

#include "tachyon/core/presets/sfx/sfx_params.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/api.h"

namespace tachyon { class DiagnosticBag; }
namespace tachyon::media { class IAssetResolver; }

namespace tachyon::core::presets::sfx {

namespace spec = tachyon::spec;

/**
 * @brief Build an AudioTrackSpec for a specific SFX preset.
 */
[[nodiscard]] TACHYON_API spec::AudioTrackSpec build_sfx(
    const media::IAssetResolver& resolver, 
    const SfxParams& p, 
    ::tachyon::DiagnosticBag* diagnostics = nullptr);

[[nodiscard]] TACHYON_API spec::AudioTrackSpec build_sfx(
    const media::IAssetResolver& resolver, 
    SfxCategory cat, 
    double trigger_time, 
    float volume = 1.0f, 
    ::tachyon::DiagnosticBag* diagnostics = nullptr);

} // namespace tachyon::core::presets::sfx
