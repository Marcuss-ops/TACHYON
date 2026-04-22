#pragma once
#include "tachyon/core/api.h"
#include <cstdint>
#include <string>
#include <variant>

namespace tachyon {

/**
 * @brief Type-safe data binding for composition parameters.
 * 
 * Replaces 'magic strings' with explicit typed references.
 */
struct DataBindingSpec {
    enum class DataType : std::uint8_t {
        Scalar,
        Vector2,
        Vector3,
        Color,
        String
    };

    DataType type{DataType::Scalar};
    std::uint32_t data_source_index{0}; ///< Index into a DataSnapshot.
    
    // Versioning for incremental invalidation
    std::uint32_t min_compatible_version{1};
};

struct CompiledDataBinding {
    std::uint32_t target_property_id;
    DataBindingSpec binding;
};

} // namespace tachyon
