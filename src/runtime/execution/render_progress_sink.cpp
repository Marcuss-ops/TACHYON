#include "tachyon/runtime/execution/render_progress_sink.h"
#include <iostream>

namespace tachyon {

namespace {

const char* phase_to_string(RenderPhase phase) {
    switch (phase) {
        case RenderPhase::CompileScene: return "CompileScene";
        case RenderPhase::BuildRenderPlan: return "BuildRenderPlan";
        case RenderPhase::BuildExecutionPlan: return "BuildExecutionPlan";
        case RenderPhase::InitializeSession: return "InitializeSession";
        case RenderPhase::Render: return "Render";
        default: return "Unknown";
    }
}

} // anonymous namespace

void ConsoleRenderProgressSink::on_phase_start(RenderPhase phase, const std::string& description) {
    std::cout << "[NativeRender] Starting " << phase_to_string(phase);
    if (!description.empty()) {
        std::cout << ": " << description;
    }
    std::cout << "\n";
}

void ConsoleRenderProgressSink::on_phase_complete(RenderPhase phase, double elapsed_ms) {
    std::cout << "[NativeRender] Completed " << phase_to_string(phase);
    if (elapsed_ms > 0.0) {
        std::cout << " (" << elapsed_ms << " ms)";
    }
    std::cout << "\n";
}

void ConsoleRenderProgressSink::on_message(const std::string& message) {
    std::cout << "[NativeRender] " << message << "\n";
}

} // namespace tachyon
