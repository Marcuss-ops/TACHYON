#include "tachyon/scene/builder.h"

namespace tachyon::scene {

// SceneBuilder implementation
SceneBuilder::SceneBuilder() {
    spec_.project.id = "new_project";
    spec_.project.name = "New Project";
}

SceneBuilder::SceneBuilder(std::string id, std::string name) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
}

SceneBuilder& SceneBuilder::project(std::string id, std::string name) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
    return *this;
}

SceneBuilder& SceneBuilder::authoring_tool(std::string tool) {
    spec_.project.authoring_tool = std::move(tool);
    return *this;
}

SceneBuilder& SceneBuilder::root_seed(std::int64_t seed) {
    spec_.project.root_seed = seed;
    return *this;
}

SceneBuilder& SceneBuilder::composition(std::string id, std::function<void(CompositionBuilder&)> fn) {
    CompositionBuilder cb(std::move(id));
    fn(cb);
    spec_.compositions.push_back(std::move(cb).build());
    return *this;
}

SceneSpec SceneBuilder::build() {
    return std::move(spec_);
}

CompositionBuilder Composition(std::string id) {
    return CompositionBuilder(std::move(id));
}

SceneBuilder Scene() {
    return SceneBuilder();
}

} // namespace tachyon::scene
