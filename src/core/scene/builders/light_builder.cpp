#include "tachyon/scene/light_builder.h"
#include "tachyon/scene/builder.h"

namespace tachyon::scene {

LightBuilder& LightBuilder::type(std::string t) {
    parent_.spec_.type = LayerType::Light;
    parent_.spec_.light_type = std::move(t);
    return *this;
}

LightBuilder& LightBuilder::color(const ColorSpec& c) {
    parent_.spec_.type = LayerType::Light;
    parent_.spec_.light_color.value = c;
    return *this;
}

LightBuilder& LightBuilder::intensity(double i) {
    parent_.spec_.type = LayerType::Light;
    parent_.spec_.light_intensity = anim::scalar(i);
    return *this;
}

LightBuilder& LightBuilder::casts_shadows(bool v) {
    parent_.spec_.casts_shadows = v;
    return *this;
}

LightBuilder& LightBuilder::shadow_radius(double r) {
    parent_.spec_.shadow_radius = anim::scalar(r);
    return *this;
}

LayerBuilder& LightBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
