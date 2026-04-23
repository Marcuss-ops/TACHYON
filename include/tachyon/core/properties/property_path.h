#pragma once

#include <string>
#include <vector>

namespace tachyon::properties {

/**
 * @brief Standardized property paths for the Tachyon engine.
 * 
 * Used by the Graph Editor, Expression Engine, and Scene Serialization
 * to uniquely identify animatable properties.
 */
struct PropertyPath {
    static constexpr const char* kOpacity = "opacity";
    static constexpr const char* kPositionX = "transform.position_x";
    static constexpr const char* kPositionY = "transform.position_y";
    static constexpr const char* kPositionZ = "transform.position_z";
    static constexpr const char* kScaleX = "transform.scale_x";
    static constexpr const char* kScaleY = "transform.scale_y";
    static constexpr const char* kScaleZ = "transform.scale_z";
    static constexpr const char* kRotation = "transform.rotation";
    
    static constexpr const char* kMaskFeather = "mask.feather";
    
    static constexpr const char* kFocalLength = "camera.focal_length";
    static constexpr const char* kAperture = "camera.aperture";
    static constexpr const char* kFocusDistance = "camera.focus_distance";
    
    /**
     * @brief Build a full path for a layer property.
     * format: layers[layer_id].property_name
     */
    static std::string layer_property(const std::string& layer_id, const std::string& prop) {
        return "layers[" + layer_id + "]." + prop;
    }
};

} // namespace tachyon::properties
