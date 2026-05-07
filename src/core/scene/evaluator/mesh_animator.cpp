#include "tachyon/core/scene/evaluator/mesh_animator.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/media/loading/mesh_asset.h"
#include <algorithm>
#include <vector>

namespace tachyon::scene {

void evaluate_mesh_animations(EvaluatedLayerState& evaluated, const media::MeshAsset* asset_ptr, double time) {
    if (!asset_ptr) return;
    const auto& asset = *asset_ptr;
    
    struct TRS {
        math::Vector3 translation{0,0,0};
        math::Quaternion rotation = math::Quaternion::identity();
        math::Vector3 scale{1,1,1};
        bool has_t{false}, has_r{false}, has_s{false};
    };
    std::vector<TRS> joint_trs;
    if (!asset.skins.empty()) {
        joint_trs.resize(asset.skins[0].joints.size());
    }

    for (const auto& anim : asset.animations) {
        for (const auto& chan : anim.channels) {
            if (chan.times.empty()) continue;
            
            size_t idx = 0;
            if (time <= chan.times.front()) idx = 0;
            else if (time >= chan.times.back()) idx = chan.times.size() - 2;
            else {
                for (size_t i = 0; i < chan.times.size() - 1; ++i) {
                    if (time < chan.times[i+1]) { idx = i; break; }
                }
            }
            
            float t = (static_cast<float>(time) - chan.times[idx]) / (chan.times[idx+1] - chan.times[idx]);
            t = std::clamp(t, 0.0f, 1.0f);
            
            if (chan.path == media::MeshAsset::AnimationChannel::Path::Weights) {
                size_t num_morphs = chan.values.size() / chan.times.size();
                evaluated.morph_weights.resize(num_morphs, 0.0f);
                for (size_t m = 0; m < num_morphs; ++m) {
                    float v0 = chan.values[idx * num_morphs + m];
                    float v1 = chan.values[(idx+1) * num_morphs + m];
                    evaluated.morph_weights[m] = v0 * (1.0f - t) + v1 * t;
                }
            } else if (!asset.skins.empty()) {
                const auto& skin = asset.skins[0];
                int joint_idx = -1;
                for (int j = 0; j < static_cast<int>(skin.joints.size()); ++j) {
                    if (skin.joints[j].index == chan.node_idx) { joint_idx = j; break; }
                }
                
                if (joint_idx >= 0) {
                    TRS& trs = joint_trs[joint_idx];
                    if (chan.path == media::MeshAsset::AnimationChannel::Path::Translation) {
                        math::Vector3 v0{chan.values[idx*3+0], chan.values[idx*3+1], chan.values[idx*3+2]};
                        math::Vector3 v1{chan.values[(idx+1)*3+0], chan.values[(idx+1)*3+1], chan.values[(idx+1)*3+2]};
                        trs.translation = v0 * (1.0f - t) + v1 * t;
                        trs.has_t = true;
                    } else if (chan.path == media::MeshAsset::AnimationChannel::Path::Scale) {
                        math::Vector3 v0{chan.values[idx*3+0], chan.values[idx*3+1], chan.values[idx*3+2]};
                        math::Vector3 v1{chan.values[(idx+1)*3+0], chan.values[(idx+1)*3+1], chan.values[(idx+1)*3+2]};
                        trs.scale = v0 * (1.0f - t) + v1 * t;
                        trs.has_s = true;
                    } else if (chan.path == media::MeshAsset::AnimationChannel::Path::Rotation) {
                        math::Quaternion q0{chan.values[idx*4+0], chan.values[idx*4+1], chan.values[idx*4+2], chan.values[idx*4+3]};
                        math::Quaternion q1{chan.values[(idx+1)*4+0], chan.values[(idx+1)*4+1], chan.values[(idx+1)*4+2], chan.values[(idx+1)*4+3]};
                        if (math::Quaternion::dot(q0, q1) < 0) q1 = q1 * -1.0f;
                        trs.rotation = (q0 * (1.0f - t) + q1 * t).normalized();
                        trs.has_r = true;
                    }
                }
            }
        }
    }
    
    if (!asset.skins.empty()) {
        const auto& skin = asset.skins[0];
        evaluated.joint_matrices.resize(skin.joints.size());
        std::vector<math::Matrix4x4> global_transforms(skin.joints.size());
        
        for (size_t i = 0; i < skin.joints.size(); ++i) {
            const auto& joint = skin.joints[i];
            const TRS& trs = joint_trs[i];
            
            math::Matrix4x4 local;
            if (trs.has_t || trs.has_r || trs.has_s) {
                local = math::compose_trs(trs.translation, trs.rotation, trs.scale);
            } else {
                local = joint.local_transform;
            }
            
            if (joint.parent >= 0) {
                int parent_joint_idx = -1;
                for (int pj = 0; pj < static_cast<int>(skin.joints.size()); ++pj) {
                    if (skin.joints[pj].index == joint.parent) { parent_joint_idx = pj; break; }
                }
                
                if (parent_joint_idx >= 0) {
                    global_transforms[i] = global_transforms[parent_joint_idx] * local;
                } else {
                    global_transforms[i] = local;
                }
            } else {
                global_transforms[i] = local;
            }
            
            evaluated.joint_matrices[i] = global_transforms[i] * joint.inverse_bind_matrix;
        }
    }
}

} // namespace tachyon::scene
