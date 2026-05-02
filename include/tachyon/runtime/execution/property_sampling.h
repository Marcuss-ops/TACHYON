#pragma once

#include "tachyon/runtime/core/data/compiled_scene.h"

namespace tachyon::runtime {

/**
 * @brief Samples a compiled property track at a given time.
 */
float sample_compiled_property_track(const tachyon::CompiledPropertyTrack& track, float time);

} // namespace tachyon::runtime
