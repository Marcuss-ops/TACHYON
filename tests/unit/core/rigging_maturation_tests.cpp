#include "tachyon/core/scene/rigging/rig_graph.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <gtest/gtest.h>
#include <cmath>

namespace tachyon::scene::rigging {

class RiggingMaturationTest : public ::testing::Test {
protected:
    Pose pose;
    RigGraph graph;
};

TEST_F(RiggingMaturationTest, MultiPassEvaluation_FK) {
    // Parent
    PoseNode parent;
    parent.id = "parent";
    parent.local_position = {10, 0, 0};
    pose.add_node(parent);
    
    // Child
    PoseNode child;
    child.id = "child";
    child.parent_id = "parent";
    child.local_position = {5, 0, 0};
    pose.add_node(child);
    
    pose.compute_world_matrices();
    
    auto* p = pose.get_node("parent");
    auto* c = pose.get_node("child");
    
    EXPECT_TRUE(std::fabs(p->world_matrix[12] - 10.0f) < 1e-5f);
    EXPECT_TRUE(std::fabs(c->world_matrix[12] - 15.0f) < 1e-5f); // 10 + 5
}

TEST_F(RiggingMaturationTest, ConstraintSolving_Point) {
    PoseNode target;
    target.id = "target";
    target.local_position = {100, 0, 0};
    pose.add_node(target);
    
    PoseNode source;
    source.id = "source";
    source.local_position = {0, 0, 0};
    pose.add_node(source);
    
    std::vector<EvaluatedLayerState> layers;
    EvaluatedLayerState l_source; l_source.id = "source";
    ConstraintSpec constraint;
    constraint.type = ConstraintType::Point;
    constraint.weight = 1.0f;
    constraint.targets.push_back({"target", 1.0f, {0,0,0}});
    l_source.constraints.push_back(constraint);
    layers.push_back(l_source);
    
    EvaluatedLayerState l_target; l_target.id = "target";
    layers.push_back(l_target);
    
    graph.build_from_layers(layers);
    graph.evaluate(pose);
    
    auto* s = pose.get_node("source");
    EXPECT_TRUE(std::fabs(s->world_matrix[12] - 100.0f) < 1e-5f);
}

TEST_F(RiggingMaturationTest, IKSolving_CCD) {
    // root -> mid -> end
    PoseNode root; root.id = "root"; root.local_position = {0,0,0}; pose.add_node(root);
    PoseNode mid; mid.id = "mid"; mid.parent_id = "root"; mid.local_position = {10,0,0}; pose.add_node(mid);
    PoseNode end; end.id = "end"; end.parent_id = "mid"; end.local_position = {10,0,0}; pose.add_node(end);
    
    // Target at {15, 5, 0}
    PoseNode target; target.id = "target"; target.local_position = {15,5,0}; pose.add_node(target);
    
    std::vector<EvaluatedLayerState> layers;
    EvaluatedLayerState l_root;
    l_root.id = "root";
    l_root.name = "root";
    layers.push_back(l_root);

    EvaluatedLayerState l_mid;
    l_mid.id = "mid";
    l_mid.name = "mid";
    layers.push_back(l_mid);
    
    EvaluatedLayerState l_end; 
    l_end.id = "end"; 
    IKSpec ik;
    ik.id = "ik";
    ik.target_id = "target";
    ik.chain_length = 3; // end, mid, root
    ik.iterations = 50;
    ik.weight = 1.0f;
    l_end.ik_chains.push_back(ik);
    layers.push_back(l_end);
    
    EvaluatedLayerState l_target_layer;
    l_target_layer.id = "target";
    l_target_layer.name = "target";
    layers.push_back(l_target_layer);
    
    graph.build_from_layers(layers);
    graph.evaluate(pose);
    
    auto* e = pose.get_node("end");
    // End effector should be close to {15, 5, 0}
    EXPECT_TRUE(std::fabs(e->world_matrix[12] - 15.0f) < 0.1f);
    EXPECT_TRUE(std::fabs(e->world_matrix[13] - 5.0f) < 0.1f);
}

} // namespace tachyon::scene::rigging
