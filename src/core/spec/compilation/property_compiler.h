#pragma once

#include "src/core/spec/compilation/compilation_context.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/expressions/expression_engine.h"
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

} // namespace tachyon
