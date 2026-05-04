#include "tachyon/runtime/execution/property_sampling.h"
#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_interpolation.h"
#include "tachyon/core/animation/property_adapter.h"
#include <algorithm>

namespace tachyon::runtime {

float sample_compiled_property_track(const tachyon::CompiledPropertyTrack& track, float time) {
    if (track.kind == tachyon::CompiledPropertyTrack::Kind::Constant) {
        return static_cast<float>(track.constant_value);
    }
    
    if (track.keyframes.empty()) {
        return static_cast<float>(track.constant_value);
    }

    auto generic_prop = animation::to_generic(track);
    return static_cast<float>(animation::sample_keyframes(generic_prop.keyframes, track.constant_value, static_cast<double>(time), animation::lerp_scalar));
}

} // namespace tachyon::runtime
