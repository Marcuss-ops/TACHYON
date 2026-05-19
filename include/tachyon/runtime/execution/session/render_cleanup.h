#pragma once

#include "tachyon/runtime/execution/session/render_session_state.h"

namespace tachyon {

class RenderCleanup {
public:
    static void run(RenderSessionState& state);
};

} // namespace tachyon
