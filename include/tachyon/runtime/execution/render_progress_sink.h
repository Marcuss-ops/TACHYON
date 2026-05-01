#pragma once

#include <string>

namespace tachyon {

enum class RenderPhase {
    CompileScene,
    BuildRenderPlan,
    BuildExecutionPlan,
    InitializeSession,
    Render,
};

struct RenderProgressSink {
    virtual ~RenderProgressSink() = default;
    virtual void on_phase_start(RenderPhase phase, const std::string& description = {}) = 0;
    virtual void on_phase_complete(RenderPhase phase, double elapsed_ms = 0.0) = 0;
    virtual void on_message(const std::string& message) = 0;
};

// Default implementation that outputs to std::cout
struct ConsoleRenderProgressSink : RenderProgressSink {
    void on_phase_start(RenderPhase phase, const std::string& description = {}) override;
    void on_phase_complete(RenderPhase phase, double elapsed_ms = 0.0) override;
    void on_message(const std::string& message) override;
};

} // namespace tachyon
