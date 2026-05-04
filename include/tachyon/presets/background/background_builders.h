#pragma once

#include "tachyon/core/api.h"
#include "tachyon/presets/background/background_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::presets {

[[nodiscard]] TACHYON_API LayerSpec build_background(const BackgroundParams& p);

} // namespace tachyon::presets
