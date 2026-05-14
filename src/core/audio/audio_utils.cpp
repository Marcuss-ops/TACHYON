#include "tachyon/core/audio/audio_export_interface.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"

namespace tachyon::audio {

bool has_any_audio(const RenderPlan& plan) {
    if (!plan.scene_spec) return false;
    
    if (!plan.output.profile.audio.tracks.empty()) return true;
    
    // Find the composition target
    for (const auto& comp : plan.scene_spec->compositions) {
        if (comp.id == plan.composition_target) {
            return !comp.audio_tracks.empty();
        }
    }
    return false;
}

} // namespace tachyon::audio
