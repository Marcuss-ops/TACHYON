#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string_view>
#include <unordered_map>

namespace tachyon {

std::string_view to_canonical_layer_type_string(LayerType type) {
    switch (type) {
        case LayerType::Solid:      return "solid";
        case LayerType::Shape:      return "shape";
        case LayerType::Image:      return "image";
        case LayerType::Video:      return "video";
        case LayerType::Text:       return "text";
        case LayerType::Camera:      return "camera";
        case LayerType::Precomp:     return "precomp";
        case LayerType::Light:       return "light";
        case LayerType::Mask:        return "mask";
        case LayerType::NullLayer:   return "null";
        case LayerType::Procedural:  return "procedural";
        case LayerType::Mesh:        return "mesh";
        default:                     return "unknown";
    }
}

LayerType layer_type_from_string(std::string_view type_string) {
    static const std::unordered_map<std::string_view, LayerType> mapping = {
        {"solid",      LayerType::Solid},
        {"shape",      LayerType::Shape},
        {"image",      LayerType::Image},
        {"video",      LayerType::Video},
        {"text",       LayerType::Text},
        {"camera",     LayerType::Camera},
        {"precomp",    LayerType::Precomp},
        {"light",      LayerType::Light},
        {"mask",       LayerType::Mask},
        {"null",       LayerType::NullLayer},
        {"procedural", LayerType::Procedural},
        {"mesh",       LayerType::Mesh},
    };

    auto it = mapping.find(type_string);
    if (it != mapping.end()) {
        return it->second;
    }
    return LayerType::Unknown;
}

std::uint32_t compiled_type_id_from_layer_type(LayerType type) {
    if (type == LayerType::Video) {
        return static_cast<std::uint32_t>(LayerType::Image);
    }
    return static_cast<std::uint32_t>(type);
}

} // namespace tachyon
