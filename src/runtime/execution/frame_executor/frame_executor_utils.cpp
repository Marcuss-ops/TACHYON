#include "frame_executor_internal.h"
#include "tachyon/runtime/execution/property_sampling.h"
#include "tachyon/runtime/execution/rasterization_step.h"
#include <algorithm>
#include <cmath>

namespace tachyon {

std::uint64_t build_node_key(std::uint64_t global_key, const CompiledNode& node) {
    CacheKeyBuilder builder;
    builder.add_u64(global_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.topo_index);
    builder.add_u32(node.version);
    return builder.finish();
}

std::shared_ptr<renderer2d::SurfaceRGBA> render_frame_at_time(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    double render_time,
     const std::uint64_t composition_key,
     const std::uint64_t frame_key) {
    // Build a robust cache key using frame_key, render time, composition, and plan params
    CacheKeyBuilder temp_builder;
    temp_builder.add_u64(frame_key);
    temp_builder.add_f64(render_time);
    temp_builder.add_u64(composition_key);
    if (plan.composition.frame_rate.numerator > 0) {
        temp_builder.add_u64(static_cast<std::uint64_t>(plan.composition.frame_rate.numerator));
    }
    if (task.subframe_index.has_value()) {
        temp_builder.add_u64(*task.subframe_index);
    }
    if (task.motion_blur_sample_index.has_value()) {
        temp_builder.add_u64(*task.motion_blur_sample_index);
    }
    const std::uint64_t temp_key = temp_builder.finish();

    // Create a modified task with the target time
    FrameRenderTask temp_task = task;
    temp_task.time_seconds = render_time;

    // Evaluate the scene at the specified time
    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        if (context.cancel_flag && context.cancel_flag->load()) break;
        evaluate_node(executor, node_id, compiled_scene, plan, snapshot, context, composition_key, temp_key, render_time, temp_task);
    }

    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        const std::uint64_t root_key = build_node_key(temp_key, root_comp.node);
        evaluate_composition(executor, compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, temp_key, render_time, temp_task);
    }

    // Render the evaluated composition
    auto cached_comp = executor.cache().lookup_composition(build_node_key(temp_key, compiled_scene.compositions.front().node));
    if (cached_comp) {
        RasterizedFrame2D rasterized = render_evaluated_composition_2d(*cached_comp, plan, temp_task, context.renderer2d);
        return rasterized.surface;
    }
    return nullptr;
}

timeline::FrameBuffer surface_to_framebuffer(const renderer2d::SurfaceRGBA& src) {
    timeline::FrameBuffer dst;
    dst.width = src.width();
    dst.height = src.height();
    dst.channels = 4; // RGBA
    dst.data.resize(static_cast<size_t>(dst.width) * dst.height * dst.channels);

    const std::size_t dst_width = static_cast<std::size_t>(dst.width);
    const std::size_t dst_height = static_cast<std::size_t>(dst.height);
    const std::size_t dst_channels = static_cast<std::size_t>(dst.channels);
    for (std::size_t y = 0; y < dst_height; ++y) {
        for (std::size_t x = 0; x < dst_width; ++x) {
            const auto pixel = src.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            const size_t idx = (y * dst_width + x) * dst_channels;
            dst.data[idx + 0] = static_cast<uint8_t>(pixel.r * 255.0f);
            dst.data[idx + 1] = static_cast<uint8_t>(pixel.g * 255.0f);
            dst.data[idx + 2] = static_cast<uint8_t>(pixel.b * 255.0f);
            dst.data[idx + 3] = static_cast<uint8_t>(pixel.a * 255.0f);
        }
    }
    return dst;
}

std::shared_ptr<renderer2d::Framebuffer> framebuffer_to_framebuffer(const timeline::FrameBuffer& src) {
    auto dst = std::make_shared<renderer2d::Framebuffer>(src.width, src.height);
    const std::size_t src_width = static_cast<std::size_t>(src.width);
    const std::size_t src_height = static_cast<std::size_t>(src.height);
    const std::size_t src_channels = static_cast<std::size_t>(src.channels);
    for (std::size_t y = 0; y < src_height; ++y) {
        for (std::size_t x = 0; x < src_width; ++x) {
            const size_t idx = (y * src_width + x) * src_channels;
            renderer2d::Color c(
                src.data[idx + 0] / 255.0f,
                src.data[idx + 1] / 255.0f,
                src.data[idx + 2] / 255.0f,
                src.data[idx + 3] / 255.0f);
            dst->set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), c);
        }
    }
    return dst;
}

} // namespace tachyon
