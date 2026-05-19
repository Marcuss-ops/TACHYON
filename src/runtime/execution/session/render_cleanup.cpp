#include "tachyon/runtime/execution/session/render_cleanup.h"
#include "tachyon/runtime/execution/session/audio_export_step.h"

namespace tachyon {

void RenderCleanup::run(RenderSessionState& state) {
    AudioExportStep::cleanup_temporary_audio(state);
}

} // namespace tachyon
