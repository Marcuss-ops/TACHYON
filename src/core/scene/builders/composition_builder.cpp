#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"

namespace tachyon::scene {

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
        [](LayerBuilder& l) { l.transform3d().is_3d(true).done(); l.camera().type("two_node").done(); }, fn);
}

CompositionBuilder& CompositionBuilder::light_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id),
        [](LayerBuilder& l) { l.transform3d().is_3d(true).done(); l.light().type("point").done(); }, fn);
}

CompositionBuilder& CompositionBuilder::mesh_layer(std::string id, std::function<void(LayerBuilder&)> fn) {
    return add_typed_layer(std::move(id),
        [](LayerBuilder& l) { l.transform3d().is_3d(true).done(); l.type(LayerType::Shape); }, fn);
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

} // namespace tachyon::scene
