#include "tachyon/runtime/execution/render_progress_sink.h"
#include <spdlog/fmt/fmt.h>

namespace tachyon {

namespace {

const char* phase_to_string(RenderPhase phase) {
    switch (phase) {
        case RenderPhase::CompileScene:        return "CompileScene";
        case RenderPhase::BuildRenderPlan:     return "BuildRenderPlan";
        case RenderPhase::BuildExecutionPlan:  return "BuildExecutionPlan";
        case RenderPhase::InitializeSession:   return "InitializeSession";
        case RenderPhase::Render:              return "Render";
        default:                               return "Unknown";
    }
}

} // anonymous namespace

void ConsoleRenderProgressSink::on_phase_start(RenderPhase phase, const std::string& description) {
    if (description.empty()) {
        fmt::print("[NativeRender] Starting {}\n", phase_to_string(phase));
    } else {
        fmt::print("[NativeRender] Starting {}: {}\n", phase_to_string(phase), description);
    }
}

void ConsoleRenderProgressSink::on_phase_complete(RenderPhase phase, double elapsed_ms) {
    if (elapsed_ms > 0.0) {
        fmt::print("[NativeRender] Completed {} ({:.1f} ms)\n", phase_to_string(phase), elapsed_ms);
    } else {
        fmt::print("[NativeRender] Completed {}\n", phase_to_string(phase));
    }
}

void ConsoleRenderProgressSink::on_message(const std::string& message) {
    fmt::print("[NativeRender] {}\n", message);
}

} // namespace tachyon
