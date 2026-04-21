#pragma once
#include "tachyon/core/api.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon {

/**
 * @brief Represents a snapshot of expression properties and state at a specific time.
 * 
 * DataSnapshots are the primary mechanism for fine-grained invalidation. 
 * By comparing snapshot hashes, the engine can determine exactly which layers 
 * or effects need re-rendering.
 */
struct TACHYON_ALIGN(16) DataSnapshot {
    std::uint64_t timestamp_ms{0};     ///< The time point this snapshot represents.
    std::uint64_t scene_hash{0};       ///< Hash of the CompiledScene this snapshot belongs to.
    
    struct PropertyValue {
        double value{0.0};
        bool is_computed{false};
    };

    /**
     * @brief Map of property indices to evaluated values.
     * Capturing this allows "ValueAtTime" and inter-frame dependencies to 
     * skip VM evaluation.
     */
    std::unordered_map<std::uint32_t, PropertyValue> computed_properties;
    
    std::vector<std::byte> opaque_state; ///< External data connector state (JSON/Binary).
};

} // namespace tachyon
