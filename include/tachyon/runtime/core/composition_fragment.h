#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tachyon {

struct CompiledComposition;

/**
 * @brief Represents a partial composition that can be instantiated or linked.
 * Foundational for component factories and modular scene authoring.
 */
class CompositionFragment {
public:
    std::string fragment_id;
    std::string name;
    
    // Links to the actual compiled data
    std::shared_ptr<CompiledComposition> template_composition;
    
    struct ParameterMapping {
        std::string param_name;
        std::uint32_t target_property_index;
    };
    
    std::vector<ParameterMapping> interface_parameters;
};

} // namespace tachyon
