#include "frame_executor_internal.h"

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

    if (executor.m_cache.lookup_composition(node_key)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->composition_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->composition_misses++;
        context.diagnostic_tracker->compositions_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->width = comp.width;
    state->height = comp.height;
    state->frame_rate.numerator = comp.fps;
    state->frame_rate.denominator = 1;
    state->frame_number = task.frame_number;
    state->composition_time_seconds = frame_time_seconds;

    state->layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        const std::uint64_t layer_key = build_node_key(frame_key, layer.node);
        auto cached_layer = executor.m_cache.lookup_layer(layer_key);
        if (cached_layer) {
            state->layers.push_back(*cached_layer);
        }
    }

    executor.m_cache.store_composition(node_key, std::move(state));
}

} // namespace tachyon
