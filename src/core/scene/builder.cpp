#include "tachyon/scene/builder.h"
#include <algorithm>
#include <cmath>
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/presets/text/text_builders.h"
#include "tachyon/presets/text/text_animator_preset_registry.h"

namespace tachyon::scene {

namespace expr {

AnimatedScalarSpec wiggle(double frequency, double amplitude, int seed) {
    AnimatedScalarSpec spec;
    spec.wiggle.enabled = true;
    spec.wiggle.frequency = frequency;
    spec.wiggle.amplitude = amplitude;
    spec.wiggle.seed = seed;
    return spec;
}

AnimatedScalarSpec sin_wave(double frequency, double amplitude, double offset) {
    AnimatedScalarSpec spec;
    const double duration = 10.0;
    const int samples = static_cast<int>(duration * 30.0);
    for (int i = 0; i < samples; ++i) {
        double t = i / 30.0;
        double val = std::sin(t * frequency * 2.0 * 3.14159265358979323846 + offset) * amplitude;
        spec.keyframes.push_back({static_cast<double>(i), val});
    }
    return spec;
}

AnimatedScalarSpec pulse(double frequency, double amplitude) {
    AnimatedScalarSpec spec;
    const double duration = 10.0;
    for (int i = 0; i < static_cast<int>(duration * 30.0); ++i) {
        double t = i / 30.0;
        double val = (std::fmod(t * frequency, 1.0) < 0.5) ? amplitude : 0.0;
        spec.keyframes.push_back({static_cast<double>(i), val});
    }
    return spec;
}

} // namespace expr

namespace anim {
AnimatedScalarSpec scalar(double v) {
    AnimatedScalarSpec s;
    s.keyframes.push_back({0.0, v});
    return s;
}

AnimatedScalarSpec lerp(double from, double to, double duration, animation::EasingPreset ease) {
    AnimatedScalarSpec s;
    s.keyframes.push_back({0.0, from});
        // Simplification: just two keyframes. In a real engine this might need easing info.
        // We assume the evaluator handles interpolation.
        s.keyframes.push_back({duration * 1000.0, to});
        // Note: Engine uses frames or ms? Original used double duration.
        // Let's assume frames for now if int64, but the engine usually handles time mapping.
        return s;
    }

AnimatedScalarSpec keyframes(std::initializer_list<std::pair<double, double>> time_value_pairs) {
    AnimatedScalarSpec s;
    for (auto p : time_value_pairs) {
            s.keyframes.push_back({p.first * 1000.0, p.second});
    }
    return s;
}

    AnimatedScalarSpec from_fn(double duration, std::function<double(double t)> fn, int samples) {
    AnimatedScalarSpec s;
    for (int i = 0; i <= samples; ++i) {
        double t = (duration * i) / samples;
        s.keyframes.push_back({t * 1000.0, fn(t)});
    }
    return s;
}
}

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

// LayerBuilder implementation
LayerBuilder::LayerBuilder(std::string id) {
    spec_.id = std::move(id);
}

LayerBuilder::LayerBuilder(LayerSpec spec) : spec_(std::move(spec)) {}

LayerBuilder& LayerBuilder::type(std::string t) {
    spec_.type = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::kind(LayerType t) {
    spec_.kind = t;
    return *this;
}

LayerBuilder& LayerBuilder::solid(std::string name) {
    spec_.kind = LayerType::Solid;
    spec_.type = "solid";
    spec_.asset_id = std::move(name);
    return *this;
}

LayerBuilder& LayerBuilder::image(std::string path) {
    spec_.kind = LayerType::Image;
    spec_.type = "image";
    spec_.asset_id = std::move(path);
    return *this;
}

LayerBuilder& LayerBuilder::mesh(std::string path) {
    spec_.kind = LayerType::Shape;
    spec_.type = "mesh";
    spec_.asset_id = std::move(path);
    return *this;
}

LayerBuilder& LayerBuilder::preset(std::string name) {
    spec_.type = "preset";
    spec_.preset_id = std::move(name);
    return *this;
}

LayerBuilder& LayerBuilder::text(std::string t) {
    spec_.kind = LayerType::Text;
    spec_.text_content = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::font(std::string f) {
    spec_.font_id = std::move(f);
    return *this;
}

LayerBuilder& LayerBuilder::font_size(double sz) {
    spec_.font_size = sz;
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
    spec_.width = static_cast<std::int64_t>(w);
    spec_.height = static_cast<std::int64_t>(h);
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
    spec_.kind = LayerType::NullLayer;
    spec_.type = "null";
    return *this;
}

LayerBuilder& LayerBuilder::precomp(std::string composition_id) {
    spec_.kind = LayerType::Precomp;
    spec_.type = "precomp";
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

LayerBuilder& LayerBuilder::text_animator(const TextAnimatorSpec& anim) {
    spec_.text_animators.push_back(anim);
    return *this;
}

LayerBuilder& LayerBuilder::text_animators(std::vector<TextAnimatorSpec> anims) {
    spec_.text_animators.insert(spec_.text_animators.end(), anims.begin(), anims.end());
    return *this;
}

LayerBuilder& LayerBuilder::text_highlight(const TextHighlightSpec& hl) {
    spec_.text_highlights.push_back(hl);
    return *this;
}

LayerBuilder& LayerBuilder::text_highlights(std::vector<TextHighlightSpec> hls) {
    spec_.text_highlights.insert(spec_.text_highlights.end(), hls.begin(), hls.end());
    return *this;
}

LayerBuilder& LayerBuilder::subtitle_path(std::string path) {
    spec_.subtitle_path = std::move(path);
    return *this;
}

LayerBuilder& LayerBuilder::text_animation_preset(const std::string& id) {
    spec_.text_animators = presets::TextAnimatorPresetRegistry::instance().create(id, registry::ParameterBag{});
    return *this;
}

LayerBuilder& LayerBuilder::transition_in_preset(const std::string& id, double duration) {
    registry::ParameterBag bag;
    bag.set("duration", duration);
    spec_.transition_in = presets::TransitionPresetRegistry::instance().create(id, bag);
    return *this;
}

LayerBuilder& LayerBuilder::transition_out_preset(const std::string& id, double duration) {
    registry::ParameterBag bag;
    bag.set("duration", duration);
    spec_.transition_out = presets::TransitionPresetRegistry::instance().create(id, bag);
    return *this;
}

LayerBuilder& LayerBuilder::light_leak(LightLeakPreset preset, float progress, float speed, int seed) {
    EffectSpec leak;
    leak.type = "light_leak";
    leak.scalars["preset"] = static_cast<double>(static_cast<int>(preset));
    leak.scalars["progress"] = static_cast<double>(progress);
    leak.scalars["speed"] = static_cast<double>(speed);
    leak.scalars["seed"] = static_cast<double>(seed);
    spec_.effects.push_back(leak);
    return *this;
}

LayerBuilder& LayerBuilder::position3d(double x, double y, double z) {
    spec_.is_3d = true;
    spec_.transform3d.position_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

LayerBuilder& LayerBuilder::rotation3d(double x, double y, double z) {
    spec_.is_3d = true;
    spec_.transform3d.rotation_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

LayerBuilder& LayerBuilder::scale3d(double x, double y, double z) {
    spec_.is_3d = true;
    spec_.transform3d.scale_property.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

LayerBuilder& LayerBuilder::is_3d(bool v) {
    spec_.is_3d = v;
    return *this;
}

LayerBuilder& LayerBuilder::camera_type(std::string t) {
    spec_.kind = LayerType::Camera;
    spec_.camera_type = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::camera_poi(double x, double y, double z) {
    spec_.kind = LayerType::Camera;
    spec_.camera_poi.value = math::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    return *this;
}

LayerBuilder& LayerBuilder::camera_zoom(double z) {
    spec_.kind = LayerType::Camera;
    spec_.camera_zoom = anim::scalar(z);
    return *this;
}

LayerBuilder& LayerBuilder::light_type(std::string t) {
    spec_.kind = LayerType::Light;
    spec_.light_type = std::move(t);
    return *this;
}

LayerBuilder& LayerBuilder::light_color(const ColorSpec& c) {
    spec_.kind = LayerType::Light;
    spec_.light_color.value = c;
    return *this;
}

LayerBuilder& LayerBuilder::light_intensity(double i) {
    spec_.kind = LayerType::Light;
    spec_.light_intensity = anim::scalar(i);
    return *this;
}

LayerBuilder& LayerBuilder::casts_shadows(bool v) {
    spec_.casts_shadows = v;
    return *this;
}

LayerBuilder& LayerBuilder::shadow_radius(double r) {
    spec_.shadow_radius = anim::scalar(r);
    return *this;
}

LayerBuilder& LayerBuilder::emission_strength(double v) {
    spec_.emission_strength = anim::scalar(v);
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

// CompositionBuilder implementation
CompositionBuilder::CompositionBuilder(std::string id) {
    spec_.id = std::move(id);
    spec_.name = spec_.id;
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

CompositionBuilder& CompositionBuilder::background(BackgroundSpec background) {
    spec_.background = std::move(background);
    return *this;
}

CompositionBuilder& CompositionBuilder::background_preset(const std::string& id, double duration) {
    registry::ParameterBag bag;
    bag.set("width", static_cast<int>(spec_.width));
    bag.set("height", static_cast<int>(spec_.height));
    bag.set("duration", duration);
    if (auto bg_layer = presets::BackgroundPresetRegistry::instance().create(id, bag)) {
        spec_.layers.insert(spec_.layers.begin(), std::move(*bg_layer));
    }
    return *this;
}

CompositionBuilder& CompositionBuilder::clear(const ColorSpec& color) {
    spec_.background = BackgroundSpec{};
    spec_.background->type = BackgroundType::Color;
    spec_.background->parsed_color = color;
    spec_.background->value = "solid_color";
    return *this;
}

CompositionBuilder& CompositionBuilder::add_typed_layer(
    std::string id,
    std::function<void(LayerBuilder&)> defaults,
    std::function<void(LayerBuilder&)> fn) {
    LayerBuilder lb(std::move(id));
    defaults(lb);
    fn(lb);
    spec_.layers.push_back(std::move(lb).build());
    return *this;
}

CompositionBuilder& CompositionBuilder::layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    LayerBuilder lb(std::move(id));
    fn(lb);
    spec_.layers.push_back(std::move(lb).build());
    return *this;
}

CompositionBuilder& CompositionBuilder::layer(const LayerSpec& layer) {
    spec_.layers.push_back(layer);
    return *this;
}

CompositionBuilder& CompositionBuilder::null_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id), [](LayerBuilder& l) { l.null_layer(); }, fn);
}

CompositionBuilder& CompositionBuilder::precomp_layer(std::string id, std::string composition_id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id), [&](LayerBuilder& l) { l.precomp(std::move(composition_id)); }, fn);
}

CompositionBuilder& CompositionBuilder::camera3d_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id),
        [](LayerBuilder& l) { l.is_3d(true); l.type("camera"); l.kind(LayerType::Camera); l.camera_type("two_node"); }, fn);
}

CompositionBuilder& CompositionBuilder::light_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id),
        [](LayerBuilder& l) { l.is_3d(true); l.type("light"); l.kind(LayerType::Light); l.light_type("point"); }, fn);
}

CompositionBuilder& CompositionBuilder::mesh_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id),
        [](LayerBuilder& l) { l.is_3d(true); l.type("mesh"); l.kind(LayerType::Shape); }, fn);
}

CompositionBuilder& CompositionBuilder::audio(std::string path, double volume) {
    tachyon::spec::AudioTrackSpec track;
    track.id = "audio_" + std::to_string(spec_.audio_tracks.size());
    track.source_path = std::move(path);
    track.volume = static_cast<float>(volume);
    spec_.audio_tracks.push_back(std::move(track));
    return *this;
}

CompositionBuilder& CompositionBuilder::audio(const AudioTrackSpec& track) {
    spec_.audio_tracks.push_back(track);
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

SceneBuilder Scene() {
    return SceneBuilder{};
}

} // namespace tachyon::scene
