#include "tachyon/scene/builder.h"
#include <algorithm>
#include <utility>

namespace tachyon::scene {

namespace anim {

AnimatedScalarSpec scalar(double v) {
    return AnimatedScalarSpec(v);
}

AnimatedScalarSpec lerp(double from, double to, double duration, animation::EasingPreset ease) {
    AnimatedScalarSpec spec;
    
    ScalarKeyframeSpec k1;
    k1.time = 0.0;
    k1.value = from;
    k1.easing = ease;
    
    ScalarKeyframeSpec k2;
    k2.time = duration;
    k2.value = to;
    k2.easing = animation::EasingPreset::None;
    
    spec.keyframes.push_back(k1);
    spec.keyframes.push_back(k2);
    return spec;
}

AnimatedScalarSpec keyframes(std::initializer_list<std::pair<double, double>> time_value_pairs) {
    AnimatedScalarSpec spec;
    for (const auto& [t, v] : time_value_pairs) {
        ScalarKeyframeSpec k;
        k.time = t;
        k.value = v;
        spec.keyframes.push_back(k);
    }
    return spec;
}

AnimatedScalarSpec from_fn(double duration, std::function<double(double t)> fn, int samples) {
    AnimatedScalarSpec spec;
    if (samples < 2) samples = 2;
    
    for (int i = 0; i < samples; ++i) {
        double t_norm = static_cast<double>(i) / (samples - 1);
        double time = t_norm * duration;
        
        ScalarKeyframeSpec k;
        k.time = time;
        k.value = fn(time);
        spec.keyframes.push_back(k);
    }
    return spec;
}

} // namespace anim

// TransitionBuilder
TransitionBuilder& TransitionBuilder::id(std::string transition_id) {
    spec_.transition_id = std::move(transition_id);
    return *this;
}

TransitionBuilder& TransitionBuilder::duration(double d) {
    spec_.duration = d;
    return *this;
}

TransitionBuilder& TransitionBuilder::ease(animation::EasingPreset e) {
    spec_.easing = e;
    return *this;
}

TransitionBuilder& TransitionBuilder::delay(double d) {
    spec_.delay = d;
    return *this;
}

LayerBuilder& TransitionBuilder::done() {
    return parent_;
}

// LayerBuilder implementation
LayerBuilder::LayerBuilder(std::string id) {
    spec_.id = std::move(id);
}

LayerBuilder& LayerBuilder::type(std::string t) {
    spec_.type = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::text(std::string t) {
    spec_.text_content = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::font(std::string f) {
    spec_.font_id = std::move(f);
    return *this;
}

LayerBuilder& LayerBuilder::font_size(double sz) {
    spec_.font_size = AnimatedScalarSpec(sz);
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
    spec_.opacity = v;
    spec_.opacity_property = AnimatedScalarSpec(v);
    return *this;
}

LayerBuilder& LayerBuilder::opacity(AnimatedScalarSpec anim_spec) {
    spec_.opacity_property = std::move(anim_spec);
    return *this;
}

LayerBuilder& LayerBuilder::position(double x, double y) {
    spec_.transform.position_property.value = math::Vector2{static_cast<float>(x), static_cast<float>(y)};
    return *this;
}

LayerBuilder& LayerBuilder::color(const ColorSpec& c) {
    spec_.fill_color = AnimatedColorSpec(c);
    return *this;
}

LayerBuilder& LayerBuilder::mesh(std::string path) {
    spec_.is_3d = true;
    spec_.type = "mesh";
    this->prop("mesh_path", path);
    return *this;
}

LayerBuilder& LayerBuilder::prop(std::string key, nlohmann::json val) {
    spec_.input_props[std::move(key)] = std::move(val);
    return *this;
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

MaterialBuilder& MaterialBuilder::base_color(const ColorSpec& c) {
    parent_.spec_.fill_color = AnimatedColorSpec(c);
    return *this;
}

MaterialBuilder& MaterialBuilder::metallic(double v) {
    parent_.spec_.metallic = AnimatedScalarSpec(v);
    return *this;
}

MaterialBuilder& MaterialBuilder::roughness(double v) {
    parent_.spec_.roughness = AnimatedScalarSpec(v);
    return *this;
}

LayerBuilder& MaterialBuilder::done() {
    return parent_;
}

// CompositionBuilder implementation
CompositionBuilder::CompositionBuilder(std::string id) {
    spec_.id = std::move(id);
}

CompositionBuilder& CompositionBuilder::size(int w, int h) {
    spec_.width = w;
    spec_.height = h;
    return *this;
}

CompositionBuilder& CompositionBuilder::fps(int f) {
    spec_.fps = f;
    spec_.frame_rate.numerator = f;
    spec_.frame_rate.denominator = 1;
    return *this;
}

CompositionBuilder& CompositionBuilder::duration(double d) {
    spec_.duration = d;
    return *this;
}

CompositionBuilder& CompositionBuilder::background(std::string color_or_preset) {
    spec_.background = BackgroundSpec::from_string(color_or_preset);
    return *this;
}

CompositionBuilder& CompositionBuilder::layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    LayerBuilder lb(std::move(id));
    fn(lb);
    spec_.layers.push_back(std::move(lb).build());
    return *this;
}

CompositionBuilder& CompositionBuilder::audio(std::string path, double volume) {
    tachyon::spec::AudioTrackSpec track;
    track.id = "audio_" + std::to_string(spec_.audio_tracks.size());
    track.source_path = std::move(path);
    track.volume = static_cast<float>(volume);
    spec_.audio_tracks.push_back(std::move(track));
    return *this;
}

CompositionSpec CompositionBuilder::build() && {
    return std::move(spec_);
}

CompositionSpec CompositionBuilder::build() const & {
    return spec_;
}

SceneSpec CompositionBuilder::build_scene() {
    SceneSpec scene;
    scene.compositions.push_back(std::move(spec_));
    return scene;
}

// SceneBuilder implementation
SceneBuilder& SceneBuilder::project(std::string id, std::string name) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
    return *this;
}

SceneBuilder& SceneBuilder::composition(std::string id, std::function<void(CompositionBuilder&)> fn) {
    CompositionBuilder cb(std::move(id));
    fn(cb);
    spec_.compositions.push_back(std::move(cb).build());
    return *this;
}

SceneSpec SceneBuilder::build() {
    return std::move(spec_);
}

CompositionBuilder Composition(std::string id) {
    return CompositionBuilder(std::move(id));
}

} // namespace tachyon::scene
