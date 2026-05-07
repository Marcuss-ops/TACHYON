#include "tachyon/runtime/compiler/component_library.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>

namespace tachyon {

void ComponentLibrary::add_component(ComponentSpec component) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_components.push_back(std::move(component));
}

const ComponentSpec* ComponentLibrary::find_component(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& comp : m_components) {
        if (comp.id == id) return &comp;
    }
    return nullptr;
}

void ComponentLibrary::merge(const ComponentLibrary& other) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Note: We don't lock 'other' because we assume it's stable or we only have const access.
    // However, if 'other' is also a ComponentLibrary, we should probably lock it too if we want full safety.
    // But since find_component also locks m_mutex, we have to be careful about deadlocks.
    // For now, let's just lock this instance.
    for (const auto& comp : other.components()) {
        bool found = false;
        for (const auto& existing : m_components) {
            if (existing.id == comp.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_components.push_back(comp);
        }
    }
}

ComponentLibrary& ComponentLibrary::global() {
    static ComponentLibrary instance;
    return instance;
}

} // namespace tachyon
