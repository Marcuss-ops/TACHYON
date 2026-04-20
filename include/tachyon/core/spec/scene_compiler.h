#pragma once

#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/runtime/core/compiled_scene.h"
#include "tachyon/runtime/core/diagnostics.h"

namespace tachyon {

struct SceneCompilerOptions {
    DeterminismPolicy determinism;
};

class SceneCompiler {
public:
    explicit SceneCompiler(SceneCompilerOptions options = {});

    [[nodiscard]] ResolutionResult<CompiledScene> compile(const SceneSpec& scene) const;

private:
    SceneCompilerOptions m_options;
};

} // namespace tachyon

