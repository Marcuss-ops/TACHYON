#include "tachyon/runtime/core/diagnostics.h"
#include "tachyon/runtime/core/determinism_contract.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/runtime/core/compiled_scene.h"

namespace tachyon {

struct SceneCompilerOptions {
    DeterminismContract determinism;
};

class SceneCompiler {
public:
    explicit SceneCompiler(SceneCompilerOptions options = {});

    [[nodiscard]] ResolutionResult<CompiledScene> compile(const SceneSpec& scene) const;

    /**
     * @brief Incrementally updates an existing compiled scene based on a new specification.
     * This preserves steady-state data and execution graph topology when possible.
     */
    bool update_compiled_scene(CompiledScene& existing, const SceneSpec& new_spec) const;

private:
    SceneCompilerOptions m_options;
};

} // namespace tachyon
