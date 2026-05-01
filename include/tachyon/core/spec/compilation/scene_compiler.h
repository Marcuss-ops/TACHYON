#pragma once
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/core/contracts/determinism_contract.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include <filesystem>

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

    /**
     * @brief Loads an external scene (USD, Alembic) and returns a SceneSpec.
     */
    static ResolutionResult<SceneSpec> import_external_scene(const std::filesystem::path& path);

private:
    SceneCompilerOptions m_options;
};

} // namespace tachyon
