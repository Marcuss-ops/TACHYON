#include "tachyon/runtime/compiler/property_compiler.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <type_traits>

namespace tachyon {

template<typename T>
CompiledPropertyTrack PropertyCompiler::compile_track(
    CompilationRegistry& registry,
    const std::string& id_suffix,
    const std::string& layer_id,
    const T& property_spec,
    double fallback_value,
    std::vector<expressions::Bytecode>& expressions,
    DiagnosticBag& diagnostics) {
    
    CompiledPropertyTrack track;
    track.node = registry.create_node(CompiledNodeType::Property);
    track.property_id = layer_id + id_suffix;

    if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
        if (property_spec.value.has_value()) {
            if (id_suffix.find("_x") != std::string::npos) track.constant_value = property_spec.value->x;
            else if (id_suffix.find("_y") != std::string::npos) track.constant_value = property_spec.value->y;
            else track.constant_value = fallback_value;
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
            ck.spring_stiffness = keyframe.spring.stiffness;
            ck.spring_damping = keyframe.spring.damping;
            ck.spring_mass = keyframe.spring.mass;

            // If it's a custom easing and we have a next keyframe, we can compute the AE-style Bezier
            if (keyframe.easing == animation::EasingPreset::Custom && i + 1 < property_spec.keyframes.size()) {
                const auto& next_kf = property_spec.keyframes[i + 1];
                double next_val = 0.0;
                if constexpr (std::is_same_v<T, AnimatedVector2Spec>) {
                    if (id_suffix.find("_x") != std::string::npos) next_val = next_kf.value.x;
                    else if (id_suffix.find("_y") != std::string::npos) next_val = next_kf.value.y;
                } else {
                    next_val = static_cast<double>(next_kf.value);
                }

                double duration = next_kf.time - keyframe.time;
                double delta = next_val - val;
                
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
        auto compile_result = expressions::CoreExpressionEvaluator::compile(*property_spec.expression);
        if (compile_result.success) {
            track.expression_index = static_cast<std::uint32_t>(expressions.size());
            expressions.push_back(std::move(compile_result.bytecode));
        } else {
            track.kind = CompiledPropertyTrack::Kind::Constant;
            diagnostics.add_warning("COMPILER_W002",
                "Expression compilation failed for property '" + track.property_id +
                "', falling back to constant value. Error: " + compile_result.error,
                track.property_id);
        }
    }

    return track;
}

// Explicit instantiations
template CompiledPropertyTrack PropertyCompiler::compile_track<AnimatedScalarSpec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedScalarSpec&, double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

template CompiledPropertyTrack PropertyCompiler::compile_track<AnimatedVector2Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector2Spec&, double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

} // namespace tachyon
