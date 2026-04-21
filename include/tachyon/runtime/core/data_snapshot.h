#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon {

/**
 * @brief Represents a snapshot of expression properties and state at a specific time.
 * Used for caching, state persistence, and inter-frame evaluation.
 */
struct DataSnapshot {
    std::uint64_t timestamp_ms{0};
    std::uint64_t scene_version{0};
    
    struct PropertyValue {
        double value{0.0};
        bool is_computed{false};
    };

    std::unordered_map<std::uint32_t, PropertyValue> computed_properties;
    std::vector<std::byte> opaque_state;
};

} // namespace tachyon
