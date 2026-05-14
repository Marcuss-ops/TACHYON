#include "frame_executor_internal.h"
#include <chrono>

namespace tachyon {

void evaluate_composition(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledComposition& comp,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const std::uint64_t composition_key,
    std::uint64_t node_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task,
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    (void)composition_key;
    (void)scene;
    (void)plan;
    (void)snapshot;
    (void)context;
    (void)main_frame_key;
    (void)main_frame_time;

    const auto timing_start = std::chrono::high_resolution_clock::now();
    auto record_timing = [&]() {
        if (!context.diagnostics) return;
        const auto timing_end = std::chrono::high_resolution_clock::now();
        context.diagnostics->timings.push_back(TimingSample{
            "composition",
            std::to_string(comp.node.node_id),
            std::chrono::duration<double, std::milli>(timing_end - timing_start).count()
        });
    };

    if (executor.m_cache.lookup_composition(node_key)) {
        if (context.diagnostics) context.diagnostics->composition_hits++;
        record_timing();
        return;
    }

    if (context.diagnostics) {
        context.diagnostics->composition_misses++;
        context.diagnostics->compositions_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->composition_id = std::to_string(comp.node.node_id);
    state->width = comp.width;
    state->height = comp.height;
    state->frame_number = task.frame_number;
    state->composition_time_seconds = frame_time_seconds;
    state->duration_seconds = comp.duration;

    state->layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        const std::uint64_t layer_key = build_node_key(frame_key, layer.node);
        auto cached_layer = executor.m_cache.lookup_layer(layer_key);
        if (cached_layer) {
            state->layers.push_back(*cached_layer);
        }
    }

    executor.m_cache.store_composition(node_key, std::move(state));
    record_timing();
}

} // namespace tachyon
