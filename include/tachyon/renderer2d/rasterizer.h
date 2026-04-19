#pragma once

#include "tachyon/runtime/render_graph.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace tachyon {

struct RasterizedFrame2D {
    std::int64_t frame_number{0};
    std::int64_t width{0};
    std::int64_t height{0};
    std::size_t layer_count{0};
    std::size_t estimated_draw_ops{0};
    std::string backend_name;
    std::string cache_key;
    std::string note;
};

RasterizedFrame2D render_frame_2d_stub(const RenderPlan& plan, const FrameRenderTask& task);

} // namespace tachyon
