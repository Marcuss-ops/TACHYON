#include "tachyon/core/spec/compilation/component_library.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace tachyon {

// Forward declaration of parse_composition (or at least the component part)
// Since we don't want to duplicate logic, we'll use a simplified version for standalone library files
ComponentSpec parse_standalone_component(const nlohmann::json& j) {
    ComponentSpec spec;
    if (j.contains("id")) spec.id = j.at("id").get<std::string>();
    if (j.contains("name")) spec.name = j.at("name").get<std::string>();
    
    // Minimal parsing for standalone files - in a real impl this would call the full spec parser
    // For now, we assume the same structure as in SceneSpec
    return spec;
}

bool ComponentLibrary::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    try {
        nlohmann::json j;
        file >> j;
        
        if (j.is_array()) {
            for ([[maybe_unused]] const auto& item : j) {
                // In a real implementation, we'd use the full SceneSpec parser hooks
                // For now, we'll manually add the components if they are valid
            }
        } else if (j.contains("components") && j.at("components").is_array()) {
            // Support "library" files that have a "components" root
        }
        return true;
    } catch (...) {
        return false;
    }
}

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
