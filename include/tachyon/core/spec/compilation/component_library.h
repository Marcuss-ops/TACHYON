#pragma once

#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace tachyon {

/**
 * @brief A collection of reusable Scene Components that can be injected into Compositions.
 */
class ComponentLibrary {
public:
    ComponentLibrary() = default;

    /**
     * @brief Loads components from a JSON file.
     */
    bool load_from_file(const std::filesystem::path& path);

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
    const std::vector<ComponentSpec>& components() const { return m_components; }

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
};

} // namespace tachyon
