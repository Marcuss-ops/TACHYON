#include "tachyon/core/scene/evaluator/layer_utils.h"

namespace tachyon::scene {

LayerType map_layer_type(const std::string& type) {
    if (type == "solid") return LayerType::Solid;
    if (type == "shape") return LayerType::Shape;
    if (type == "mask") return LayerType::Mask;
    if (type == "image") return LayerType::Image;
    if (type == "video") return LayerType::Video;
    if (type == "text") return LayerType::Text;
    if (type == "camera") return LayerType::Camera;
    if (type == "precomp") return LayerType::Precomp;
    if (type == "light") return LayerType::Light;
    if (type == "null") return LayerType::NullLayer;
    return LayerType::Unknown;
}

} // namespace tachyon::scene
