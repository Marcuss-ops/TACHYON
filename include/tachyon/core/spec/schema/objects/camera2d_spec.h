#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/math/vector2.h"

namespace tachyon {

struct Camera2DSpec {
    std::string id;
    std::string name{"2D Camera"};
    
    bool enabled{true};
    bool visible{true};
    
    AnimatedScalarSpec zoom;
    AnimatedVector2Spec position;
    AnimatedScalarSpec rotation;
    AnimatedVector2Spec scale;
    AnimatedVector2Spec anchor_point;
    AnimatedScalarSpec focus_distance;
    
    int viewport_width{1920};
    int viewport_height{1080};
};

} // namespace tachyon
