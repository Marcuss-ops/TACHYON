#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/templates.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include <algorithm>

namespace tachyon::scene {

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& spec,
    std::size_t layer_index,
    double time_offset) {
    
    // 1. Timing & Playback Base
    double composition_time = context.composition_time_seconds + time_offset;
    
    EvaluatedLayerState state;
    state.identity.layer_id = spec.identity.id;
    state.identity.id = spec.identity.id;
    state.identity.name = spec.identity.name;
    state.identity.type = spec.identity.type;
    state.identity.enabled = spec.identity.enabled;
    state.identity.visible = spec.identity.visible;
    state.identity.is_adjustment_layer = spec.identity.is_adjustment_layer;
    state.identity.motion_blur = spec.identity.motion_blur;
    
    state.identity.active = (composition_time >= spec.playback.timing.start && 
                             composition_time < (spec.playback.timing.start + spec.playback.timing.duration));
    
    // Local time (relative to layer start)
    double layer_local_time = composition_time - spec.playback.timing.start;
    
    // Audio bands for expressions/bindings
    ::tachyon::audio::AudioBands bands = context.audio_analyzer ? context.audio_analyzer->analyze_frame(composition_time) : ::tachyon::audio::AudioBands{};

    // Time Remap
    double sampled_local_time = layer_local_time;
    if (!spec.playback.temporal.time_remap_property.empty()) {
        sampled_local_time = sample_scalar(spec.playback.temporal.time_remap_property, layer_local_time, layer_local_time, bands);
    }
    
    state.playback.local_time_seconds = sampled_local_time;
    state.playback.in_time = spec.playback.timing.start;
    state.playback.out_time = spec.playback.timing.start + spec.playback.timing.duration;
    state.playback.transition_in = spec.transition_in;
    state.playback.transition_out = spec.transition_out;

    // Seeds for expressions
    uint64_t layer_seed = make_property_expression_seed(context.scene, context.composition, spec, "layer");

    // 2. Transform Sampling
    state.transform.width = spec.transform.width;
    state.transform.height = spec.transform.height;
    
    // Opacity
    state.transform.opacity = static_cast<float>(sample_scalar(
        spec.transform.opacity_property, 
        spec.transform.opacity, 
        sampled_local_time, 
        bands,
        hash_combine(layer_seed, stable_string_hash("opacity")),
        context.vars.numeric,
        context.vars.tables,
        static_cast<uint32_t>(layer_index)));
    
    // Position, Scale, Rotation
    math::Vector2 pos = sample_vector2(
        spec.transform.transform.position_property,
        {static_cast<float>(spec.transform.transform.position_x.value_or(0.0)), static_cast<float>(spec.transform.transform.position_y.value_or(0.0))},
        sampled_local_time,
        bands,
        hash_combine(layer_seed, stable_string_hash("position")),
        context.vars.numeric,
        context.vars.tables);
    
    math::Vector2 scale = sample_vector2(
        spec.transform.transform.scale_property,
        {static_cast<float>(spec.transform.transform.scale_x.value_or(100.0)), static_cast<float>(spec.transform.transform.scale_y.value_or(100.0))},
        sampled_local_time,
        bands,
        hash_combine(layer_seed, stable_string_hash("scale")),
        context.vars.numeric,
        context.vars.tables);
        
    double rotation = sample_scalar(
        spec.transform.transform.rotation_property,
        spec.transform.transform.rotation.value_or(0.0),
        sampled_local_time,
        bands,
        hash_combine(layer_seed, stable_string_hash("rotation")),
        context.vars.numeric,
        context.vars.tables);
        
    math::Vector2 anchor = sample_vector2(
        spec.transform.transform.anchor_point,
        spec.transform.transform.anchor_point.value.value_or(math::Vector2::zero()),
        sampled_local_time,
        bands,
        hash_combine(layer_seed, stable_string_hash("anchor")),
        context.vars.numeric,
        context.vars.tables);

    // Build local matrix
    state.transform.local_transform.position = pos;
    state.transform.local_transform.scale = {scale.x / 100.0f, scale.y / 100.0f};
    state.transform.local_transform.rotation_rad = static_cast<float>(rotation * 3.14159265358979323846 / 180.0);
    state.transform.local_transform.anchor_point = anchor;
    
    state.transform.world_matrix = state.transform.local_transform.to_matrix();
    
    switch (spec.blend_mode) {
        case BlendMode::Normal: state.transform.blend_mode = "normal"; break;
        case BlendMode::Additive: state.transform.blend_mode = "additive"; break;
        case BlendMode::Multiply: state.transform.blend_mode = "multiply"; break;
        case BlendMode::Screen:   state.transform.blend_mode = "screen"; break;
        case BlendMode::Overlay:  state.transform.blend_mode = "overlay"; break;
        default: state.transform.blend_mode = "normal"; break;
    }

    // 3. Text
    state.text.content = resolve_template(spec.text.content, context.vars.strings, context.vars.numeric);
    state.text.font_id = spec.text.font_id;
    state.text.font_size = static_cast<float>(sample_scalar(spec.text.font_size, 24.0, sampled_local_time, bands));
    state.text.fill_color = sample_color(spec.text.fill_color, spec.text.fill_color.value.value_or(ColorSpec{255,255,255,255}), sampled_local_time);
    state.text.stroke_color = sample_color(spec.text.stroke_color, spec.text.stroke_color.value.value_or(ColorSpec{0,0,0,0}), sampled_local_time);
    state.text.stroke_width = static_cast<float>(sample_scalar(spec.text.stroke_width_property, spec.text.stroke_width, sampled_local_time, bands));
    state.text.box = spec.text.box;
    state.text.animators = spec.text_animators;
    state.text.highlights = spec.text_highlights;
    
    // 4. Source
    state.source.definition = spec.source;
    state.source.loop = spec.playback.loop;

    // 5. Effects
    for (const auto& effect_spec : spec.effects) {
        state.effects.push_back(effect_spec.evaluate(sampled_local_time));
    }
    
    // 6. Track Matte
    state.track_matte_type = spec.track_matte_type;

    return state;
}

} // namespace tachyon::scene
