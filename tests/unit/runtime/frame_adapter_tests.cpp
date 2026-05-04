#include "tachyon/runtime/execution/frame_adapter.h"

#include <cmath>
#include <iostream>

namespace {

bool nearly_equal(double a, double b) {
    return std::abs(a - b) < 1e-9;
}

} // namespace

bool run_frame_adapter_tests() {
    using namespace tachyon;
    using namespace tachyon::runtime;

    CompositionSpec composition;
    composition.id = "main";
    composition.duration = 10.0;
    composition.frame_rate = FrameRate{30, 1};

    NativeSceneFrameAdapter adapter(composition);

    FrameContext context;
    context.composition_id = "main";
    context.duration_seconds = 10.0;
    context.fps = 30.0;
    adapter.init(context);

    if (adapter.duration_frames() != 300) {
        std::cerr << "frame_adapter: expected 300 frames\n";
        return false;
    }

    adapter.seek_frame(90);
    if (adapter.current_frame() != 90 || !nearly_equal(adapter.current_time_seconds(), 3.0)) {
        std::cerr << "frame_adapter: seek_frame(90) should land at 3s\n";
        return false;
    }

    adapter.seek_frame(-5);
    if (adapter.current_frame() != 0 || !nearly_equal(adapter.current_time_seconds(), 0.0)) {
        std::cerr << "frame_adapter: negative seek should clamp to zero\n";
        return false;
    }

    adapter.destroy();
    if (adapter.current_frame() != 0 || !nearly_equal(adapter.current_time_seconds(), 0.0)) {
        std::cerr << "frame_adapter: destroy should reset state\n";
        return false;
    }

    return true;
}
