#include "tachyon/scene/camera_builder.h"
#include "tachyon/scene/builder.h"

namespace tachyon::scene {

CameraBuilder& CameraBuilder::type(std::string t) {
    parent_.spec_.type = LayerType::Camera;
    parent_.spec_.camera_type = std::move(t);
    return *this;
}

CameraBuilder& CameraBuilder::poi(double x, double y, double z) {
    parent_.spec_.type = LayerType::Camera;
    parent_.spec_.camera_poi.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

CameraBuilder& CameraBuilder::zoom(double z) {
    parent_.spec_.type = LayerType::Camera;
    parent_.spec_.camera_zoom = anim::scalar(z);
    return *this;
}

LayerBuilder& CameraBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
