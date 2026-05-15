#pragma once

#include <string>
#include <map>
#include <optional>

namespace tachyon::core::profiles {

/**
 * @brief Represents a set of execution and rendering parameters.
 * 
 * Replaces fragmented CLI flags with unified profiles (e.g. "preview", "final").
 */
struct ExecutionProfile {
    std::string name;
    
    // Rendering parameters
    int width{1920};
    int height{1080};
    double fps{30.0};
    std::string quality{"medium"};
    
    // Backend preferences
    std::string video_encoder{"default"};
    std::string audio_analyzer{"default"};
    std::string transition_kernel{"auto"};
    
    // Performance and Cache
    bool cache_enabled{true};
    bool jit_optimization{true};
    
    // Custom metadata
    std::map<std::string, std::string> metadata;

    static ExecutionProfile preview();
    static ExecutionProfile final();
};

} // namespace tachyon::core::profiles
