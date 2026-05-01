#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/spec/compilation/compilation_context.h"
#include <string>
#include <unordered_map>

namespace tachyon::detail {

void build_compositions(const SceneSpec& scene, CompiledScene& compiled, CompilationRegistry& registry, DiagnosticBag& diagnostics);

void resolve_dependencies(const SceneSpec& scene, CompiledScene& compiled, CompilationRegistry& registry);

void compile_graph(CompiledScene& compiled, CompilationRegistry& registry);

} // namespace tachyon::detail
