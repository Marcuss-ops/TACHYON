#pragma once
#include "tachyon/core/api.h"
#include <string>
#include <vector>
#include <memory>

namespace tachyon {

struct CompiledComposition;

/**
 * @brief A modular, reusable segment of a composition.
 * 
 * Fragments allow for instancing and composition-in-composition patterns.
 * This is the foundational unit for Phase 3 "Component Factories".
 */
class TACHYON_API CompositionFragment {
public:
    std::string fragment_id; ///< UUID or unique ID for the fragment template.
    std::string name;        ///< Human-readable name.
    
    /**
     * @brief Template composition that this fragment instantiates.
     * Shared across all instances of the same fragment.
     */
    std::shared_ptr<CompiledComposition> template_composition;
    
    /**
     * @brief Mapping between fragment parameters and internal property tracks.
     */
    struct ParameterMapping {
        std::string param_name;
        std::uint32_t target_property_index;
    };
    
    std::vector<ParameterMapping> interface_parameters;
};

} // namespace tachyon
