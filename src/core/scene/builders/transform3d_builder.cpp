#include "tachyon/scene/transform3d_builder.h"
#include "tachyon/scene/builder.h"
#include "tachyon/presets/animation3d/animation3d_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"

namespace tachyon::scene {

Transform3DBuilder& Transform3DBuilder::position(double x, double y, double z) {
    parent_.spec_.is_3d = true;
    parent_.spec_.transform3d.position_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

Transform3DBuilder& Transform3DBuilder::rotation(double x, double y, double z) {
    parent_.spec_.is_3d = true;
    parent_.spec_.transform3d.rotation_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

Transform3DBuilder& Transform3DBuilder::scale(double x, double y, double z) {
    parent_.spec_.is_3d = true;
    parent_.spec_.transform3d.scale_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

Transform3DBuilder& Transform3DBuilder::is_3d(bool v) {
    parent_.spec_.is_3d = v;
    return *this;
}

Transform3DBuilder& Transform3DBuilder::modifier(const std::string& id, const std::map<std::string, double>& scalars) {
    parent_.spec_.is_3d = true;
    if (!parent_.spec_.three_d.has_value()) {
        parent_.spec_.three_d = ThreeDSpec{};
        parent_.spec_.three_d->enabled = true;
    }
    ThreeDModifierSpec mod;
    mod.type = id;
    for (const auto& [k, v] : scalars) {
        mod.scalar_params[k] = anim::scalar(v);
    }
    parent_.spec_.three_d->modifiers.push_back(std::move(mod));
    return *this;
}

Transform3DBuilder& Transform3DBuilder::animation_preset(const std::string& id, double duration, double delay, double intensity) {
    parent_.spec_.is_3d = true;
    registry::ParameterBag bag;
    bag.set("duration", duration);
    bag.set("delay", delay);
    bag.set("intensity", intensity);
    presets::Animation3DPresetRegistry::instance().apply(id, parent_.spec_, bag);
    return *this;
}

LayerBuilder& Transform3DBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
