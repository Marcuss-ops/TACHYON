#include "tachyon/core/scene/rigging/rig_graph.h"

#include <algorithm>

namespace tachyon::scene {

void solve_constraints(std::vector<EvaluatedLayerState>& layers) {
    if (layers.empty()) return;

    const bool has_rigging_data = std::any_of(layers.begin(), layers.end(), [](const auto& layer) {
        return !layer.constraints.empty() || !layer.ik_chains.empty() || !layer.joint_matrices.empty();
    });
    if (!has_rigging_data) {
        return;
    }

    rigging::Pose initial_pose;
    for (const auto& layer : layers) {
        rigging::PoseNode node;
        node.id = layer.id;
        node.local_matrix = layer.local_transform.to_matrix();
        node.world_matrix = layer.world_matrix;
        node.world_position = layer.world_position3;
        
        node.world_scale.x = math::Vector3(layer.world_matrix[0], layer.world_matrix[4], layer.world_matrix[8]).length();
        node.world_scale.y = math::Vector3(layer.world_matrix[1], layer.world_matrix[5], layer.world_matrix[9]).length();
        node.world_scale.z = math::Vector3(layer.world_matrix[2], layer.world_matrix[6], layer.world_matrix[10]).length();
        
        node.world_normal = layer.world_normal;
        
        initial_pose.add_node(node);
    }

    rigging::RigGraph graph;
    graph.build_from_layers(layers);
    graph.evaluate(initial_pose);
    graph.apply_to_layers(initial_pose, layers);
}

} // namespace tachyon::scene
