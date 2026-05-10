#pragma once

#include <string>
#include <functional>

namespace tachyon::renderer2d {
    class SurfaceRGBA;
}

namespace tachyon::core::transition {

/**
 * @brief Registry for transition fast-paths to avoid circular dependencies between Core and Renderer.
 */
class TransitionFastPathRegistry {
public:
    using FastPathFn = std::function<bool(
        const std::string& transition_id,
        renderer2d::SurfaceRGBA& output,
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress,
        int thread_count)>;

    static void set_handler(FastPathFn handler) {
        get_handler_internal() = std::move(handler);
    }

    static bool apply(
        const std::string& transition_id,
        renderer2d::SurfaceRGBA& output,
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress,
        int thread_count) {
        auto& handler = get_handler_internal();
        if (handler) {
            return handler(transition_id, output, from, to, progress, thread_count);
        }
        return false;
    }

private:
    static FastPathFn& get_handler_internal() {
        static FastPathFn handler;
        return handler;
    }
};

} // namespace tachyon::core::transition
