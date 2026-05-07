#include "tachyon/scene/builder.h"

namespace tachyon::scene {

// MaterialBuilder
MaterialBuilder& MaterialBuilder::base_color(const ColorSpec& c) {
    parent_.spec_.fill_color.value = c;
    return *this;
}

MaterialBuilder& MaterialBuilder::metallic(double v) {
    parent_.spec_.metallic = anim::scalar(v);
    return *this;
}

MaterialBuilder& MaterialBuilder::roughness(double v) {
    parent_.spec_.roughness = anim::scalar(v);
    return *this;
}

MaterialBuilder& MaterialBuilder::transmission(double v) {
    parent_.spec_.transmission = anim::scalar(v);
    return *this;
}

MaterialBuilder& MaterialBuilder::ior(double v) {
    parent_.spec_.ior = anim::scalar(v);
    return *this;
}

LayerBuilder& MaterialBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
