#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/importer/scene_importer.h"
#include "src/core/spec/compilation/compilation_context.h"
#include "src/core/spec/compilation/layer_compiler.h"
#include "tachyon/core/spec/compilation/scene_compiler_detail.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace tachyon {

// Declarations for functions defined in scene_compiler_hash.cpp
std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract);
std::uint64_t hash_scene_structure(const SceneSpec& scene);

SceneCompiler::SceneCompiler(SceneCompilerOptions options)
    : m_options(std::move(options)) {}

ResolutionResult<CompiledScene> SceneCompiler::compile(const SceneSpec& scene_input) const {
    SceneSpec scene = flatten_scene(scene_input);
    
    ResolutionResult<CompiledScene> result;
    CompiledScene compiled;
    CompilationRegistry registry;

    compiled.header.version = 1;
    compiled.header.flags = static_cast<std::uint16_t>(m_options.determinism.fp_mode == DeterminismContract::FloatingPointMode::Strict ? 1U : 0U);
    compiled.header.layout_version = m_options.determinism.layout_version;
    compiled.header.compiler_version = m_options.determinism.compiler_version;
    compiled.header.layout_checksum = CompiledScene::calculate_layout_checksum();
    compiled.determinism = m_options.determinism;
    compiled.project_id = scene.project.id;
    compiled.project_name = scene.project.name;
    compiled.scene_hash = hash_scene_spec(scene, m_options.determinism);

    // 1. Build Compositions and Layers
    detail::build_compositions(scene, compiled, registry, result.diagnostics);

    // 2. Resolve Multi-Node Dependencies (Parenting, Track Mattes, Precomps)
    detail::resolve_dependencies(scene, compiled, registry);

    // 3. Compile Graph and Assign Topological Indices
    detail::compile_graph(compiled, registry);

    compiled.assets = scene.assets;
    compiled.scene_structure_hash = hash_scene_structure(scene);
    compiled.link_dependency_nodes();
    result.value = std::move(compiled);
    return result;
}

ResolutionResult<SceneSpec> SceneCompiler::import_external_scene(const std::filesystem::path& path) {
    ResolutionResult<SceneSpec> result;
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    std::unique_ptr<importer::SceneImporter> imp;
    if (ext == ".usd" || ext == ".usda" || ext == ".usdc") {
        imp = importer::create_usd_importer();
    } else if (ext == ".abc") {
        imp = importer::create_alembic_importer();
    }

    if (!imp) {
        result.diagnostics.add_error("IMPORT_ERR", "No importer available for format: " + ext);
        return result;
    }

    if (!imp->load(path.string())) {
        result.diagnostics.add_error("IMPORT_ERR", "Failed to load scene: " + imp->last_error());
        return result;
    }

    result.value = imp->build_scene_spec();
    return result;
}

} // namespace tachyon
