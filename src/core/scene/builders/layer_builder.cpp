#include "tachyon/scene/builder.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/presets/animation2d/animation2d_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"

namespace tachyon::scene {

// LayerBuilder implementation
LayerBuilder::LayerBuilder(std::string id) {
    spec_.id = std::move(id);
}

LayerBuilder::LayerBuilder(LayerSpec spec) : spec_(std::move(spec)) {}

LayerBuilder& LayerBuilder::type(LayerType t) {
    spec_.type = t;
    return *this;
}

LayerBuilder& LayerBuilder::solid(std::string name) {
    return type(LayerType::Solid).asset_id(std::move(name));
}

LayerBuilder& LayerBuilder::image(std::string path) {
    return type(LayerType::Image).asset_id(std::move(path));
}

LayerBuilder& LayerBuilder::mesh(std::string path) {
    return type(LayerType::Mesh).asset_id(std::move(path));
}

LayerBuilder& LayerBuilder::preset(std::string name) {
    type(LayerType::Procedural);
    spec_.preset_id = std::move(name);
    return *this;
}

LayerBuilder& LayerBuilder::asset_id(std::string id) {
    spec_.asset_id = std::move(id);
    return *this;
}

LayerBuilder& LayerBuilder::mesh_deform_id(std::optional<std::string> id) {
    spec_.mesh_deform_id = std::move(id);
    return *this;
}

LayerBuilder& LayerBuilder::clear_mesh_deform_id() {
    spec_.mesh_deform_id.reset();
    return *this;
}

LayerBuilder& LayerBuilder::in(double t) {
    spec_.in_point = t;
    return *this;
}

LayerBuilder& LayerBuilder::out(double t) {
    spec_.out_point = t;
    return *this;
}

LayerBuilder& LayerBuilder::opacity(double v) {
    spec_.opacity_property = anim::scalar(v);
    return *this;
}

LayerBuilder& LayerBuilder::opacity(const AnimatedScalarSpec& anim_spec) {
    spec_.opacity_property = anim_spec;
    return *this;
}

LayerBuilder& LayerBuilder::position(double x, double y) {
    spec_.transform.position_x = x;
    spec_.transform.position_y = y;
    return *this;
}

LayerBuilder& LayerBuilder::size(double w, double h) {
    const auto clamp_to_int = [](double value) -> int {
        const double bounded = std::clamp(
            value,
            static_cast<double>(std::numeric_limits<int>::min()),
            static_cast<double>(std::numeric_limits<int>::max()));
        return static_cast<int>(bounded);
    };

    spec_.width = clamp_to_int(w);
    spec_.height = clamp_to_int(h);
    return *this;
}

LayerBuilder& LayerBuilder::color(const ColorSpec& c) {
    spec_.fill_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::fill_color(const ColorSpec& c) {
    spec_.fill_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::stroke_color(const ColorSpec& c) {
    spec_.stroke_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::stroke_width(double w) {
    spec_.stroke_width = w;
    spec_.stroke_width_property = anim::scalar(w);
    return *this;
}

LayerBuilder& LayerBuilder::null_layer() {
    return type(LayerType::NullLayer);
}

LayerBuilder& LayerBuilder::precomp(std::string composition_id) {
    type(LayerType::Precomp);
    spec_.precomp_id = std::move(composition_id);
    return *this;
}

LayerBuilder& LayerBuilder::adjustment(bool enabled) {
    spec_.is_adjustment_layer = enabled;
    return *this;
}

LayerBuilder& LayerBuilder::track_matte(std::string layer_id, TrackMatteType type) {
    spec_.track_matte_layer_id = std::move(layer_id);
    spec_.track_matte_type = type;
    return *this;
}

LayerBuilder& LayerBuilder::parent(std::string parent_id) {
    spec_.parent = std::move(parent_id);
    return *this;
}

LayerBuilder& LayerBuilder::motion_blur(bool enabled) {
    spec_.motion_blur = enabled;
    return *this;
}

LayerBuilder& LayerBuilder::animation2d_preset(const std::string& id, double duration, double delay) {
    registry::ParameterBag bag;
    bag.set("duration", duration);
    bag.set("delay", delay);
    presets::Animation2DPresetRegistry::instance().apply(id, spec_, bag);
    return *this;
}

// Domain builder accessors
CameraBuilder LayerBuilder::camera() {
    spec_.type = LayerType::Camera;
    return CameraBuilder(*this);
}

LightBuilder LayerBuilder::light() {
    spec_.type = LayerType::Light;
    return LightBuilder(*this);
}

TextBuilder LayerBuilder::text() {
    spec_.type = LayerType::Text;
    return TextBuilder(*this);
}

EffectBuilder LayerBuilder::effects() {
    return EffectBuilder(*this);
}

Transform3DBuilder LayerBuilder::transform3d() {
    spec_.is_3d = true;
    return Transform3DBuilder(*this);
}

TransitionBuilder LayerBuilder::enter() {
    return TransitionBuilder(spec_.transition_in, *this);
}

TransitionBuilder LayerBuilder::exit() {
    return TransitionBuilder(spec_.transition_out, *this);
}

MaterialBuilder LayerBuilder::material() {
    spec_.is_3d = true;
    return MaterialBuilder(*this);
}

LayerSpec LayerBuilder::build() && {
    return std::move(spec_);
}

LayerSpec LayerBuilder::build() const & {
    return spec_;
}

} // namespace tachyon::scene
