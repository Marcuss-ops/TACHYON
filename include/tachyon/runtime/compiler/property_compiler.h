#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/compiler/scene_compiler_internal.h"
#include "tachyon/core/expressions/expression_vm.h"
#include <vector>
#include <string>

namespace tachyon {

class PropertyCompiler {
public:
    template<typename T>
    static CompiledPropertyTrack compile_track(
        CompilationRegistry& registry,
        const std::string& id_suffix,
        const std::string& layer_id,
        const T& property_spec,
        double fallback_value,
        std::vector<expressions::Bytecode>& expressions,
        DiagnosticBag& diagnostics);
};

// Explicit instantiations
extern template CompiledPropertyTrack PropertyCompiler::compile_track<AnimatedScalarSpec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedScalarSpec&, double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

extern template CompiledPropertyTrack PropertyCompiler::compile_track<AnimatedVector2Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector2Spec&, double, std::vector<expressions::Bytecode>&, DiagnosticBag&);

} // namespace tachyon
