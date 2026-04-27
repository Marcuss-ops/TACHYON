#include "frame_executor_internal.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/text/content/word_timestamps.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include <filesystem>

namespace tachyon {

namespace {

scene::LayerType resolve_layer_type(std::uint32_t type_id) {
    switch (type_id) {
        case 1: return scene::LayerType::Solid;
        case 2: return scene::LayerType::Shape;
        case 3: return scene::LayerType::Image;
        case 4: return scene::LayerType::Text;
        case 5: return scene::LayerType::Precomp;
        case 6: return scene::LayerType::Procedural;
        default: return scene::LayerType::NullLayer;
    }
}

ShapePathSpec to_shape_path_spec(const std::optional<ShapePathSpec>& spec) {
    ShapePathSpec result;
    if (!spec.has_value()) return result;
    result.closed = spec->closed;
    result.points = spec->points;
    return result;
}

constexpr double kPi = 3.14159265358979323846;

} // namespace

void evaluate_layer(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledLayer& layer,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t composition_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    (void)composition_key;
    (void)main_frame_time;
    (void)plan;
    (void)context;
    (void)snapshot;

    const bool is_sub_frame = main_frame_key.has_value();
    const bool layer_motion_blur = (layer.flags & 0x10) != 0;

    if (is_sub_frame && !layer_motion_blur) {
        const std::uint64_t main_layer_key = build_node_key(*main_frame_key, layer.node);
        if (auto cached_main = executor.m_cache.lookup_layer(main_layer_key)) {
            executor.m_cache.store_layer(build_node_key(frame_key, layer.node), cached_main);
            return;
        }
    }

    const std::uint64_t node_key = build_node_key(frame_key, layer.node);
    if (executor.m_cache.lookup_layer(node_key)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->layer_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->layer_misses++;
        context.diagnostic_tracker->layers_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->layer_index = layer.node.topo_index;
    state->id = std::to_string(layer.node.node_id);
    state->type = resolve_layer_type(layer.type_id);

    state->enabled = (layer.flags == 0U) || ((layer.flags & 0x01U) != 0U);
    state->visible = (layer.flags == 0U) || ((layer.flags & 0x02U) != 0U);
    state->is_3d = (layer.flags & 0x04U) != 0U;
    state->is_adjustment_layer = (layer.flags & 0x08U) != 0U;

    state->width = layer.width;
    state->height = layer.height;
    state->blend_mode = "normal";
    state->local_time_seconds = frame_time_seconds;
    state->child_time_seconds = frame_time_seconds;
    state->opacity = 1.0;
    state->local_transform = math::Transform2::identity();
    state->fill_color = layer.fill_color;
    state->stroke_color = layer.stroke_color;
    state->stroke_width = layer.stroke_width;
    state->line_cap = layer.line_cap;
    state->line_join = layer.line_join;
    state->miter_limit = layer.miter_limit;
    state->text_content = layer.text_content;
    state->font_id = layer.font_id;
    state->procedural = layer.procedural;
    state->font_size = layer.font_size;
    state->text_alignment = layer.text_alignment;
    state->extrusion_depth = layer.extrusion_depth;
    state->bevel_size = layer.bevel_size;
    state->text_animators = layer.text_animators;
    state->text_highlights = layer.text_highlights;
    state->subtitle_path = layer.subtitle_path;
    // Sample animated subtitle outline color to static ColorSpec
    if (layer.subtitle_outline_color.value.has_value()) {
        state->subtitle_outline_color = layer.subtitle_outline_color.value;
    } else {
        state->subtitle_outline_color = std::nullopt;
    }
    state->subtitle_outline_width = layer.subtitle_outline_width;
    state->word_timestamp_path = layer.word_timestamp_path;
    state->shape_path = to_shape_path_spec(layer.shape_path);
    state->shape_spec = layer.shape_spec;
    state->effects = layer.effects;
    state->animated_effects = layer.animated_effects;
    state->precomp_id = layer.precomp_index.has_value() ? std::make_optional(std::to_string(*layer.precomp_index)) : std::nullopt;
    state->track_matte_type = layer.matte_type;
    state->track_matte_layer_index = layer.matte_layer_index.has_value()
        ? std::make_optional(static_cast<std::size_t>(*layer.matte_layer_index))
        : std::nullopt;

    if (layer.precomp_index.has_value() && *layer.precomp_index < scene.compositions.size()) {
        const auto& nested_comp = scene.compositions[*layer.precomp_index];
        const std::uint64_t nested_key = build_node_key(frame_key, nested_comp.node);
        if (auto cached_nested = executor.m_cache.lookup_composition(nested_key)) {
            state->nested_composition = std::make_shared<scene::EvaluatedCompositionState>(*cached_nested);
        }
    }

    auto sample_property = [&](std::size_t index, double fallback) -> double {
        if (index >= layer.property_indices.size()) return fallback;
        const auto& track = scene.property_tracks[layer.property_indices[index]];
        
        CacheKeyBuilder prop_builder;
        if (track.kind == CompiledPropertyTrack::Kind::Constant) {
            prop_builder.add_u32(track.node.node_id);
            prop_builder.add_u32(0xCA57);
        } else {
            const std::uint64_t prop_node_key = build_node_key(frame_key, track.node);
            prop_builder.add_u64(prop_node_key);
            prop_builder.add_f64(frame_time_seconds);
        }
        
        const std::uint64_t prop_cache_key = prop_builder.finish();
        double sampled = fallback;
        if (executor.m_cache.lookup_property(prop_cache_key, sampled)) {
            if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
            return sampled;
        }
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_misses++;
        return fallback;
    };

    state->opacity = static_cast<float>(sample_property(0, 1.0));
    state->local_transform.position.x = static_cast<float>(sample_property(1, 0.0));
    state->local_transform.position.y = static_cast<float>(sample_property(2, 0.0));
    state->local_transform.scale.x = static_cast<float>(sample_property(3, 1.0));
    state->local_transform.scale.y = static_cast<float>(sample_property(4, 1.0));
    state->local_transform.rotation_rad = static_cast<float>(sample_property(5, 0.0) * (kPi / 180.0f));
    state->mask_feather = static_cast<float>(sample_property(6, 0.0));
    state->local_transform.anchor_point.x = static_cast<float>(sample_property(7, 0.0));
    state->local_transform.anchor_point.y = static_cast<float>(sample_property(8, 0.0));

    // --- Transition Logic ---
    const double layer_duration = layer.out_time - layer.in_time;
    const double relative_time = frame_time_seconds - layer.in_time;
    
    // Inbound transition
    if (layer.transition_in.type != "none" && relative_time < layer.transition_in.duration + layer.transition_in.delay) {
        double t = (relative_time - layer.transition_in.delay) / layer.transition_in.duration;
        t = std::clamp(t, 0.0, 1.0);
        t = animation::apply_easing(t, layer.transition_in.easing, {}, layer.transition_in.spring);
        
        if (layer.transition_in.type == "fade") {
            state->opacity *= static_cast<float>(t);
        } else if (layer.transition_in.type == "slide") {
            float offset_x = 0.0f, offset_y = 0.0f;
            if (layer.transition_in.direction == "left") offset_x = static_cast<float>(scene.compositions[0].width) * (1.0f - static_cast<float>(t));
            else if (layer.transition_in.direction == "right") offset_x = -static_cast<float>(scene.compositions[0].width) * (1.0f - static_cast<float>(t));
            else if (layer.transition_in.direction == "up") offset_y = static_cast<float>(scene.compositions[0].height) * (1.0f - static_cast<float>(t));
            else if (layer.transition_in.direction == "down") offset_y = -static_cast<float>(scene.compositions[0].height) * (1.0f - static_cast<float>(t));
            state->local_transform.position.x += offset_x;
            state->local_transform.position.y += offset_y;
        } else if (layer.transition_in.type == "zoom") {
            state->local_transform.scale.x *= static_cast<float>(t);
            state->local_transform.scale.y *= static_cast<float>(t);
        }
    }
    
    // Outbound transition
    const double time_until_end = layer.out_time - frame_time_seconds;
    if (layer.transition_out.type != "none" && time_until_end < layer.transition_out.duration) {
        double t = time_until_end / layer.transition_out.duration;
        t = std::clamp(t, 0.0, 1.0);
        t = animation::apply_easing(t, layer.transition_out.easing, {}, layer.transition_out.spring);
        
        if (layer.transition_out.type == "fade") {
            state->opacity *= static_cast<float>(t);
        } else if (layer.transition_out.type == "slide") {
            float offset_x = 0.0f, offset_y = 0.0f;
            float inv_t = (1.0f - static_cast<float>(t));
            if (layer.transition_out.direction == "left") offset_x = -static_cast<float>(scene.compositions[0].width) * inv_t;
            else if (layer.transition_out.direction == "right") offset_x = static_cast<float>(scene.compositions[0].width) * inv_t;
            else if (layer.transition_out.direction == "up") offset_y = -static_cast<float>(scene.compositions[0].height) * inv_t;
            else if (layer.transition_out.direction == "down") offset_y = static_cast<float>(scene.compositions[0].height) * inv_t;
            state->local_transform.position.x += offset_x;
            state->local_transform.position.y += offset_y;
        } else if (layer.transition_out.type == "zoom") {
            state->local_transform.scale.x *= static_cast<float>(t);
            state->local_transform.scale.y *= static_cast<float>(t);
        }
    }

    state->active = state->enabled && state->visible && state->opacity > 0.0
                   && frame_time_seconds >= layer.in_time && frame_time_seconds < layer.out_time;

    executor.m_cache.store_layer(node_key, std::move(state));
}

} // namespace tachyon
