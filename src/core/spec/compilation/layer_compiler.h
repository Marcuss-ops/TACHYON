#pragma once

#include "tachyon/core/spec/compilation/compilation_context.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <vector>
#include <string>

namespace tachyon {

class LayerCompiler {
public:
    static CompiledLayer compile_layer(
        CompilationRegistry& registry,
        const LayerSpec& layer,
        const std::string& layer_path,
        const SceneSpec& scene,
        CompiledScene& compiled,
        DiagnosticBag& diagnostics);
};

} // namespace tachyon
