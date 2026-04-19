#pragma once

#include "tachyon/runtime/frame_executor.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace tachyon {

struct RenderSessionResult {
    std::vector<ExecutedFrame> frames;
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};
    std::size_t frames_written{0};
    bool output_configured{false};
    std::string output_error;
};

class RenderSession {
public:
    RenderSessionResult render(const SceneSpec& scene, const RenderExecutionPlan& execution_plan, const std::filesystem::path& output_path = {});

    FrameCache& cache() { return m_cache; }
    const FrameCache& cache() const { return m_cache; }

private:
    FrameCache m_cache;
};

} // namespace tachyon
