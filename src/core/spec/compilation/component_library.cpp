#include "tachyon/core/spec/compilation/component_library.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>

namespace tachyon {

void ComponentLibrary::add_component(ComponentSpec component) {
    m_components.push_back(std::move(component));
}

const ComponentSpec* ComponentLibrary::find_component(const std::string& id) const {
    for (const auto& comp : m_components) {
        if (comp.id == id) return &comp;
    }
    return nullptr;
}

void ComponentLibrary::merge(const ComponentLibrary& other) {
    for (const auto& comp : other.m_components) {
        if (!find_component(comp.id)) {
            m_components.push_back(comp);
        }
    }
}

ComponentLibrary& ComponentLibrary::global() {
    static ComponentLibrary instance;
    return instance;
}

} // namespace tachyon
