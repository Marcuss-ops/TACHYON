#include "tachyon/runtime/execution/session/audio_export_step.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/core/audio/audio_export_interface.h"
#endif
#include <filesystem>
#include <system_error>

namespace tachyon {

void AudioExportStep::run(
    RenderSessionState& state,
    CancelFlag* cancel_flag
) {
    if (state.audio_plan.path.empty()) {
        return;
    }

    profiling::ProfileScope scope(
        state.context.profiler,
        profiling::ProfileEventType::AudioMux,
        "audio_export");

#ifdef TACHYON_ENABLE_MEDIA
    if (state.context.audio_exporter) {
        state.context.audio_exporter->export_plan_audio(
            state.effective_plan.render_plan,
            state.audio_plan.path,
            cancel_flag
        );
    }
#endif
}

void AudioExportStep::cleanup_temporary_audio(RenderSessionState& state) {
    if (!state.audio_plan.path.empty() && state.audio_plan.is_temporary) {
        std::error_code ec;
        std::filesystem::remove(state.audio_plan.path, ec);
    }
}

} // namespace tachyon
