#include "tachyon/scene/builder.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>

namespace tachyon::scene {

// LayerBuilder implementation
LayerBuilder::LayerBuilder(std::string id) 
    : spec_() {
    spec_.identity.id = std::move(id);
}

LayerBuilder::LayerBuilder(LayerSpec spec) 
    : spec_(std::move(spec)) {}

LayerBuilder& LayerBuilder::type(LayerType t) {
    spec_.identity.type = t;
    return *this;
}

LayerBuilder& LayerBuilder::solid(std::string name) {
    return type(LayerType::Solid).asset_id(std::move(name));
}

LayerBuilder& LayerBuilder::image(std::string path) {
    return type(LayerType::Image).asset_id(std::move(path));
}

LayerBuilder& LayerBuilder::video(std::string path) {
    return type(LayerType::Video).asset_id(std::move(path));
}

LayerBuilder& LayerBuilder::preset(std::string name) {
    type(LayerType::Procedural);
    spec_.source.preset_id = std::move(name);
    return *this;
}

LayerBuilder& LayerBuilder::background(const LayerSpec& spec) {
    spec_ = spec;
    return *this;
}

LayerBuilder& LayerBuilder::custom_layer(const LayerSpec& spec) {
    return background(spec);
}

LayerBuilder& LayerBuilder::asset_id(std::string id) {
    spec_.source.asset_id = std::move(id);
    return *this;
}

LayerBuilder& LayerBuilder::in(double t) {
    spec_.playback.timing.source_in = t;
    return *this;
}

LayerBuilder& LayerBuilder::out(double t) {
    spec_.playback.timing.source_out = t;
    return *this;
}

LayerBuilder& LayerBuilder::start(double t) {
    spec_.playback.timing.start = t;
    return *this;
}

LayerBuilder& LayerBuilder::duration(double d) {
    spec_.playback.timing.duration = d;
    return *this;
}

LayerBuilder& LayerBuilder::opacity(double v) {
    spec_.transform.opacity_property = anim::scalar(v);
    return *this;
}

LayerBuilder& LayerBuilder::opacity(const AnimatedScalarSpec& anim_spec) {
    spec_.transform.opacity_property = anim_spec;
    return *this;
}

LayerBuilder& LayerBuilder::position(double x, double y) {
    spec_.transform.transform.position_x = x;
    spec_.transform.transform.position_y = y;
    return *this;
}

LayerBuilder& LayerBuilder::anchor(double x, double y) {
    spec_.transform.transform.anchor_point.value = math::Vector2{static_cast<float>(x), static_cast<float>(y)};
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

    spec_.transform.width = clamp_to_int(w);
    spec_.transform.height = clamp_to_int(h);
    return *this;
}

LayerBuilder& LayerBuilder::color(const ColorSpec& c) {
    spec_.text.fill_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::fill_color(const ColorSpec& c) {
    spec_.text.fill_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::stroke_color(const ColorSpec& c) {
    spec_.text.stroke_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::stroke_width(double w) {
    spec_.text.stroke_width = static_cast<float>(w);
    spec_.text.stroke_width_property = anim::scalar(w);
    return *this;
}

LayerBuilder& LayerBuilder::null_layer() {
    return type(LayerType::NullLayer);
}

LayerBuilder& LayerBuilder::precomp(std::string composition_id) {
    type(LayerType::Precomp);
    spec_.source.precomp_id = std::move(composition_id);
    return *this;
}

LayerBuilder& LayerBuilder::adjustment(bool enabled) {
    spec_.identity.is_adjustment_layer = enabled;
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
    spec_.identity.motion_blur = enabled;
    return *this;
}

LayerBuilder& LayerBuilder::from_spec(const LayerSpec& spec) {
    spec_ = spec;
    return *this;
}

TextBuilder LayerBuilder::text() {
    spec_.identity.type = LayerType::Text;
    return TextBuilder(*this);
}

TextBuilder LayerBuilder::text(std::string content) {
    TextBuilder builder(*this);
    builder.content(std::move(content));
    return builder;
}

EffectBuilder LayerBuilder::effects() {
    return EffectBuilder(*this);
}

TransitionBuilder LayerBuilder::enter() {
    return TransitionBuilder(spec_.transition_in, *this);
}

TransitionBuilder LayerBuilder::exit() {
    return TransitionBuilder(spec_.transition_out, *this);
}

LayerSpec LayerBuilder::build() && {
    return std::move(spec_);
}

LayerSpec LayerBuilder::build() const & {
    return spec_;
}

} // namespace tachyon::scene
