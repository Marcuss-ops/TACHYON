#include "frame_executor_internal.h"
#include "tachyon/core/expressions/expression_vm.h"
#include "tachyon/core/expressions/expression_engine.h"

namespace tachyon {

void evaluate_property(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledPropertyTrack& track,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t node_key,
    double frame_time_seconds) {

    CacheKeyBuilder prop_builder;
    const bool is_constant = (track.kind == CompiledPropertyTrack::Kind::Constant);
    
    if (is_constant) {
        prop_builder.add_u32(track.node.node_id);
        prop_builder.add_u32(0xCA57);
    } else {
        prop_builder.add_u64(node_key);
        prop_builder.add_f64(frame_time_seconds);
    }
    const std::uint64_t prop_cache_key = prop_builder.finish();

    double cached_value = 0.0;
    if (executor.m_cache.lookup_property(prop_cache_key, cached_value)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->property_misses++;
        context.diagnostic_tracker->properties_evaluated++;
    }

    double value = track.constant_value;
    if (track.binding.has_value()) {
        const auto it = snapshot.computed_properties.find(track.binding->data_source_index);
        if (it != snapshot.computed_properties.end()) {
            value = it->second.value;
        }
    } else if (track.kind == CompiledPropertyTrack::Kind::Keyframed) {
        value = sample_keyframed_value(track, value, frame_time_seconds);
    } else if (track.kind == CompiledPropertyTrack::Kind::Expression && track.expression_index.has_value()) {
        const auto& bytecode = scene.expressions[track.expression_index.value()];
        expressions::ExpressionContext expr_context;
        expr_context.time = frame_time_seconds;
        value = expressions::ExpressionVM::execute(bytecode, expr_context);
    }

    (void)plan;
    executor.m_cache.store_property(prop_cache_key, value);
}

} // namespace tachyon
