#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/spec/compilation/scene_compiler_internal.h"
#include <type_traits>
#include <string_view>
#include <unordered_map>

namespace tachyon {
// Removed detail namespace for compile_property_track to allow it to be seen by SceneCompiler

template <typename VecT>
double component_from_suffix(const VecT& vec, std::string_view suffix, double fallback = 0.0) {
    if (suffix.find("_x") != std::string::npos) return static_cast<double>(vec.x);
    if (suffix.find("_y") != std::string::npos) return static_cast<double>(vec.y);
    if constexpr (std::is_same_v<VecT, math::Vector3>) {
        if (suffix.find("_z") != std::string::npos) return static_cast<double>(vec.z);
    }
    return fallback;
}

template<typename T>
CompiledPropertyTrack compile_property_track(
    CompilationRegistry& registry,
    const std::string& id_suffix, 
    const std::string& layer_id, 
    const T& property_spec, 
    double fallback_value) {
    
    CompiledPropertyTrack track;
    track.node = registry.create_node(CompiledNodeType::Property);
    track.property_id = layer_id + id_suffix;

    if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
        if (property_spec.value.has_value()) {
            track.constant_value = component_from_suffix(*property_spec.value, id_suffix, fallback_value);
        } else {
            track.constant_value = fallback_value;
        }

    } else if constexpr (std::is_same_v<T, AnimatedVector3Spec>) {
        if (property_spec.value.has_value()) {
            track.constant_value = component_from_suffix(*property_spec.value, id_suffix, fallback_value);
        } else {
            track.constant_value = fallback_value;
        }
    } else {
        track.constant_value = property_spec.value.has_value() ? static_cast<double>(*property_spec.value) : fallback_value;
    }

    if (!property_spec.keyframes.empty()) {
        track.kind = CompiledPropertyTrack::Kind::Keyframed;
        track.keyframes.reserve(property_spec.keyframes.size());
        for (std::size_t i = 0; i < property_spec.keyframes.size(); ++i) {
            const auto& keyframe = property_spec.keyframes[i];
            double val = 0.0;
            if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                if (id_suffix.find("_x") != std::string::npos) val = keyframe.value.x;
                else if (id_suffix.find("_y") != std::string::npos) val = keyframe.value.y;

            } else if constexpr (std::is_same_v<T, AnimatedVector3Spec>) {
                if (id_suffix.find("_x") != std::string::npos) val = keyframe.value.x;
                else if (id_suffix.find("_y") != std::string::npos) val = keyframe.value.y;
                else if (id_suffix.find("_z") != std::string::npos) val = keyframe.value.z;
            } else {
                val = static_cast<double>(keyframe.value);
            }

            CompiledKeyframe ck;
            ck.time = keyframe.time;
            ck.value = val;
            ck.easing = static_cast<std::uint32_t>(keyframe.easing);
            ck.cx1 = keyframe.bezier.cx1;
            ck.cy1 = keyframe.bezier.cy1;
            ck.cx2 = keyframe.bezier.cx2;
            ck.cy2 = keyframe.bezier.cy2;

            // If it's a custom easing and we have a next keyframe, we can compute the AE-style Bezier
            if (keyframe.easing == animation::EasingPreset::Custom && i + 1 < property_spec.keyframes.size()) {
                const auto& next_kf = property_spec.keyframes[i + 1];
                double next_val = 0.0;
                if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                    if (id_suffix.find("_x") != std::string::npos) next_val = next_kf.value.x;
                    else if (id_suffix.find("_y") != std::string::npos) next_val = next_kf.value.y;

                } else if constexpr (std::is_same_v<T, AnimatedVector3Spec>) {
                    if (id_suffix.find("_x") != std::string::npos) next_val = next_kf.value.x;
                    else if (id_suffix.find("_y") != std::string::npos) next_val = next_kf.value.y;
                    else if (id_suffix.find("_z") != std::string::npos) next_val = next_kf.value.z;
                } else {
                    next_val = static_cast<double>(next_kf.value);
                }

                double duration = next_kf.time - keyframe.time;
                double delta = next_val - val;

                // AE handles are often specified via speed/influence. 
                // We use from_ae to convert them if they seem to be set (influence != 0).
                if (duration > 1e-6 && (keyframe.influence_out > 0.0 || next_kf.influence_in > 0.0)) {
                    auto ae_bezier = animation::CubicBezierEasing::from_ae(
                        keyframe.speed_out, keyframe.influence_out,
                        next_kf.speed_in, next_kf.influence_in,
                        duration, delta
                    );
                    ck.cx1 = ae_bezier.cx1;
                    ck.cy1 = ae_bezier.cy1;
                    ck.cx2 = ae_bezier.cx2;
                    ck.cy2 = ae_bezier.cy2;
                }
            }

            track.keyframes.push_back(ck);
        }
    } else {
        track.kind = CompiledPropertyTrack::Kind::Constant;
    }

    if (property_spec.expression.has_value() && !property_spec.expression->empty()) {
        track.kind = CompiledPropertyTrack::Kind::Expression;
    }

    return track;
}

} // namespace tachyon
