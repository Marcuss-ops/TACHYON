#include "frame_executor_internal.h"

namespace tachyon {

void evaluate_node(
    FrameExecutor& executor,
    std::uint32_t node_id,
    const CompiledScene& scene,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t composition_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task,
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    const CompiledNode* node = nullptr;
    auto it = executor.m_node_lookup.find(node_id);
    if (it != executor.m_node_lookup.end()) {
        node = it->second;
    }
    if (node == nullptr) return;

    const std::uint64_t node_key = build_node_key(frame_key, *node);
    switch (node->type) {
        case CompiledNodeType::Property: {
            auto prop_it = executor.m_property_lookup.find(node_id);
            if (prop_it != executor.m_property_lookup.end()) {
                evaluate_property(executor, scene, *prop_it->second, plan, snapshot, context, node_key, frame_time_seconds);
            }
            break;
        }
        case CompiledNodeType::Layer: {
            auto layer_it = executor.m_layer_lookup.find(node_id);
            if (layer_it != executor.m_layer_lookup.end()) {
                evaluate_layer(executor, scene, *layer_it->second, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, main_frame_key, main_frame_time);
            }
            break;
        }
        case CompiledNodeType::Composition: {
            auto comp_it = executor.m_composition_lookup.find(node_id);
            if (comp_it != executor.m_composition_lookup.end()) {
                evaluate_composition(executor, scene, *comp_it->second, plan, snapshot, context, composition_key, node_key, frame_key, frame_time_seconds, task, main_frame_key, main_frame_time);
            }
            break;
        }
        default:
            break;
    }
}

} // namespace tachyon
