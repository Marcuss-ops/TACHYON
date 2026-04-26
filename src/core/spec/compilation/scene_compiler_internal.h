#pragma once

#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <unordered_map>

namespace tachyon {

/**
 * @brief Context used during compilation to track node IDs and dependencies.
 */
struct CompilationRegistry {
    std::uint32_t next_id{1};
    std::unordered_map<std::string, std::uint32_t> layer_id_map;
    std::unordered_map<std::string, std::uint32_t> composition_id_map;
    
    CompiledNode create_node(CompiledNodeType type) {
        CompiledNode node;
        node.node_id = next_id++;
        node.type = type;
        return node;
    }
};

template<typename T>
CompiledPropertyTrack compile_property_track(
    CompilationRegistry& registry,
    const std::string& id_suffix,
    const std::string& layer_id,
    const T& property_spec,
    double fallback_value,
    std::vector<expressions::Bytecode>& expressions,
    DiagnosticBag& diagnostics);

// Explicit instantiations
extern template CompiledPropertyTrack compile_property_track<AnimatedScalarSpec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedScalarSpec&,
    double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

extern template CompiledPropertyTrack compile_property_track<AnimatedVector2Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector2Spec&,
    double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

} // namespace tachyon
