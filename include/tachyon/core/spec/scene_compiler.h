#include "tachyon/runtime/core/determinism_contract.h"

namespace tachyon {

struct SceneCompilerOptions {
    DeterminismContract determinism;
};

class SceneCompiler {
public:
    explicit SceneCompiler(SceneCompilerOptions options = {});

    [[nodiscard]] ResolutionResult<CompiledScene> compile(const SceneSpec& scene) const;

private:
    SceneCompilerOptions m_options;
};

} // namespace tachyon

