#pragma once
#include "tachyon/core/api.h"
#include <string>
#include <functional>

namespace tachyon::renderer2d {
    class SurfaceRGBA;
}

namespace tachyon::core::transition {

/**
 * @brief Registry for transition fast-paths to avoid circular dependencies between Core and Renderer.
 */
class TACHYON_API TransitionFastPathRegistry {
public:
    using FastPathFn = std::function<bool(
        const std::string& transition_id,
        renderer2d::SurfaceRGBA& output,
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress,
        int thread_count)>;

    static void set_handler(FastPathFn handler);

    static bool apply(
        const std::string& transition_id,
        renderer2d::SurfaceRGBA& output,
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress,
        int thread_count);

private:
    static FastPathFn& get_handler_internal();
};

} // namespace tachyon::core::transition
