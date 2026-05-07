#pragma once

#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include <mutex>

namespace tachyon {

/**
 * @brief A collection of reusable Scene Components that can be injected into Compositions.
 */
class ComponentLibrary {
public:
    ComponentLibrary() = default;

    /**
     * @brief Adds a component to the library.
     */
    void add_component(ComponentSpec component);

    /**
     * @brief Finds a component by ID.
     */
    const ComponentSpec* find_component(const std::string& id) const;

    /**
     * @brief Gets all components.
     */
    const std::vector<ComponentSpec>& components() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_components; 
    }

    /**
     * @brief Merges another library into this one.
     */
    void merge(const ComponentLibrary& other);

    /**
     * @brief The default global library instance.
     */
    static ComponentLibrary& global();

private:
    std::vector<ComponentSpec> m_components;
    mutable std::mutex m_mutex;
};

} // namespace tachyon
