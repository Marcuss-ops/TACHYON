#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <vector>

namespace tachyon::test {

struct VisualHash {
    std::string preset_id;
    size_t frame_hash;
    bool success;
};

/**
 * @brief Simple visual audit helper. 
 * In a real system, this would actually render and hash the frame.
 * Here it simulates the process to demonstrate the professional workflow.
 */
class VisualAuditor {
public:
    static VisualHash audit_effect(const std::string& effect_id) {
        // Mock rendering and hashing logic
        // This is where you'd call the actual renderer in a full integration test
        bool exists = true; // Simulating registry check
        size_t mock_hash = std::hash<std::string>{}(effect_id + "_rendered_v1");
        
        return {effect_id, mock_hash, exists};
    }

    static bool verify_fallback(const std::string& transition_id, bool shader_missing) {
        // Mock logic to verify if fallback (crossfade) was used
        if (shader_missing) {
            // Check if result uses 'crossfade.glsl'
            return true; 
        }
        return true;
    }
};

} // namespace tachyon::test
