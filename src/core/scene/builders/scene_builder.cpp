#include "tachyon/scene/builder.h"

namespace tachyon::scene {

// SceneBuilder implementation
SceneBuilder& SceneBuilder::project(std::string id, std::string name) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
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
    return SceneBuilder{};
}

} // namespace tachyon::scene
