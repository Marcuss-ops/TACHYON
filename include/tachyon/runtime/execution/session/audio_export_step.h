#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"
#include "tachyon/runtime/execution/session/render_session.h"

namespace tachyon {

class AudioExportStep {
public:
    static void run(
        RenderSessionState& state,
        CancelFlag* cancel_flag
    );

    static void cleanup_temporary_audio(RenderSessionState& state);
};

} // namespace tachyon
