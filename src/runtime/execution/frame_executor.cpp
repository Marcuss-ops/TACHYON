#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/evaluated_composition_renderer.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tachyon {

namespace {

constexpr double kPi = 3.14159265358979323846;

std::uint64_t build_node_key(std::uint64_t global_key, const CompiledNode& node) {
    CacheKeyBuilder builder;
    builder.add_u64(global_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.topo_index);
    return builder.finish();
}

scene::LayerType resolve_layer_type(std::uint32_t type_id) {
    switch (type_id) {
        case 1: return scene::LayerType::Solid;
        case 2: return scene::LayerType::Shape;
        case 3: return scene::LayerType::Image;
        case 4: return scene::LayerType::Text;
        case 5: return scene::LayerType::Precomp;
        default: return scene::LayerType::NullLayer;
    }
}

scene::EvaluatedShapePath to_evaluated_shape_path(const std::optional<ShapePathSpec>& spec) {
    scene::EvaluatedShapePath evaluated;
    if (!spec.has_value()) return evaluated;

    evaluated.closed = spec->closed;
    for (const auto& point : spec->points) {
        evaluated.points.push_back(scene::EvaluatedShapePathPoint{
            point.position,
            point.tangent_in,
            point.tangent_out
        });
    }
    return evaluated;
}

double sample_keyframed_value(const CompiledPropertyTrack& track, double fallback, double t) {
    if (track.keyframes.empty()) return fallback;
    if (t <= track.keyframes.front().time) return track.keyframes.front().value;
    if (t >= track.keyframes.back().time) return track.keyframes.back().value;
    
    // Binary search for efficiency in industrial usage
    auto it = std::upper_bound(track.keyframes.begin(), track.keyframes.end(), t,
        [](double time, const CompiledKeyframe& kf) { return time < kf.time; });
    
    if (it == track.keyframes.begin() || it == track.keyframes.end()) return fallback;
    
    const auto& k0 = *(it - 1);
    const auto& k1 = *it;
    
    const double duration = k1.time - k0.time;
    if (duration <= 0.0) return k0.value;
    
    const double alpha = (t - k0.time) / duration;
    double eased_alpha = alpha;
    
    if (k0.easing == static_cast<uint32_t>(animation::EasingPreset::Custom)) { // EasingPreset::Custom
        animation::CubicBezierEasing bezier{k0.cx1, k0.cy1, k0.cx2, k0.cy2};
        eased_alpha = bezier.evaluate(alpha);
    } else if (k0.easing != static_cast<uint32_t>(animation::EasingPreset::None)) { // Not EasingPreset::None
        // Preset-based easing
        animation::CubicBezierEasing bezier;
        switch (static_cast<animation::EasingPreset>(k0.easing)) {
            case animation::EasingPreset::EaseIn:    bezier = animation::CubicBezierEasing::ease_in(); break;
            case animation::EasingPreset::EaseOut:   bezier = animation::CubicBezierEasing::ease_out(); break;
            case animation::EasingPreset::EaseInOut: bezier = animation::CubicBezierEasing::ease_in_out(); break;
            default: bezier = animation::CubicBezierEasing::linear(); break;
        }
        eased_alpha = bezier.evaluate(alpha);
    }
    
    return k0.value + eased_alpha * (k1.value - k0.value);
}

} // namespace

void FrameExecutor::build_lookup_table(const CompiledScene& scene) {
    m_node_lookup.clear();
    m_property_lookup.clear();
    m_layer_lookup.clear();
    m_composition_lookup.clear();

    for (const auto& composition : scene.compositions) {
        m_node_lookup[composition.node.node_id] = &composition.node;
        m_composition_lookup[composition.node.node_id] = &composition;

        for (const auto& layer : composition.layers) {
            m_node_lookup[layer.node.node_id] = &layer.node;
            m_layer_lookup[layer.node.node_id] = &layer;
        }
    }

    for (const auto& track : scene.property_tracks) {
        m_node_lookup[track.node.node_id] = &track.node;
        m_property_lookup[track.node.node_id] = &track;
    }
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {

    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return execute(compiled_scene, plan, task, *snapshot, context);
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context) {

    if (m_node_lookup.empty()) {
        build_lookup_table(compiled_scene);
    }

    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;

    // Detect if diagnostics are enabled
    const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
    const bool diagnostics_enabled = (diag_env && std::string_view(diag_env) == "1");

    if (diagnostics_enabled) {
        context.diagnostic_tracker = &result.diagnostics;
    }

    // Sync QualityPolicy to contexts
    context.policy = plan.quality_policy;
    context.renderer2d.policy = plan.quality_policy;

    CacheKeyBuilder comp_builder;
    comp_builder.enable_manifest(diagnostics_enabled);
    comp_builder.add_u64(compiled_scene.scene_hash);
    comp_builder.add_u32(compiled_scene.header.layout_version);
    comp_builder.add_u32(compiled_scene.header.compiler_version);
    comp_builder.add_string(plan.quality_tier);
    comp_builder.add_string(plan.compositing_alpha_mode);
    const std::uint64_t composition_key = comp_builder.finish();
    if (diagnostics_enabled) result.diagnostics.composition_key_manifest = comp_builder.manifest();

    CacheKeyBuilder frame_builder = comp_builder; // Copy structural parts
    frame_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    const std::uint64_t frame_key = frame_builder.finish();
    result.cache_key = frame_key;
    if (diagnostics_enabled) result.diagnostics.frame_key_manifest = frame_builder.manifest();

    const double fps = compiled_scene.compositions.empty()
        ? 60.0
        : std::max(1u, compiled_scene.compositions.front().fps);
    const double frame_time_seconds = static_cast<double>(task.frame_number) / fps;

    std::uint64_t root_key = 0;
    bool was_cached = false;
    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        root_key = build_node_key(frame_key, root_comp.node);
        was_cached = (m_cache.lookup_composition(root_key) != nullptr);
    }

    if (!was_cached) {
        const auto& topo_order = compiled_scene.graph.topo_order();
        for (std::uint32_t node_id : topo_order) {
            evaluate_node(node_id, compiled_scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task);
        }

        // Guarantee root composition evaluation if it was missing from topo_order
        if (!compiled_scene.compositions.empty()) {
            if (m_cache.lookup_composition(root_key) == nullptr) {
                const CompiledComposition& root_comp = compiled_scene.compositions.front();
                evaluate_composition(compiled_scene, root_comp, plan, snapshot, context, composition_key, root_key, frame_key, frame_time_seconds, task);
            }
        }
    }

    if (!compiled_scene.compositions.empty()) {
        auto cached_comp = m_cache.lookup_composition(root_key);
        if (cached_comp) {
            result.cache_hit = was_cached;

            if (plan.motion_blur_enabled && plan.motion_blur_samples > 1) {
                int samples = static_cast<int>(plan.motion_blur_samples);
                // Respect quality policy cap
                if (samples > plan.quality_policy.motion_blur_sample_cap) {
                    samples = plan.quality_policy.motion_blur_sample_cap;
                }
                
                const double shutter_duration = (plan.motion_blur_shutter_angle / 360.0) / fps;
                const double shutter_start_offset = (plan.motion_blur_shutter_phase / 360.0) / fps;

                std::unique_ptr<renderer2d::Framebuffer> accumulated;

                for (int s = 0; s < samples; ++s) {
                    const double t_offset = shutter_start_offset + (samples > 1 ? (static_cast<double>(s) / (samples - 1)) * shutter_duration : 0.0);
                    const double sub_frame_time = frame_time_seconds + t_offset;

                    // Build unique sample key for this sub-frame sample
                    CacheKeyBuilder sample_builder = comp_builder;
                    sample_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
                    sample_builder.add_f64(sub_frame_time);
                    const std::uint64_t sample_key = sample_builder.finish();
                    
                    const std::uint64_t root_sample_key = build_node_key(sample_key, compiled_scene.compositions.front().node);

                    const auto& topo_order = compiled_scene.graph.topo_order();
                    for (std::uint32_t node_id : topo_order) {
                        evaluate_node(node_id, compiled_scene, plan, snapshot, context, composition_key, sample_key, sub_frame_time, task, frame_key, frame_time_seconds);
                    }

                    // Explicitly evaluate root for this sample if missing from topo_order
                    if (m_cache.lookup_composition(root_sample_key) == nullptr) {
                         evaluate_composition(compiled_scene, compiled_scene.compositions.front(), plan, snapshot, context, composition_key, root_sample_key, sample_key, sub_frame_time, task);
                    }

                    auto sub_comp = m_cache.lookup_composition(root_sample_key);
                    if (sub_comp) {
                        RasterizedFrame2D rasterized = render_evaluated_composition_2d(*sub_comp, plan, task, context.renderer2d);
                        if (rasterized.surface) {
                            if (!accumulated) {
                                accumulated = std::make_unique<renderer2d::Framebuffer>(std::move(*rasterized.surface));
                            } else {
                                auto& acc_pixels = accumulated->mutable_pixels();
                                const auto& sub_pixels = rasterized.surface->pixels();
                                const std::size_t count = std::min(acc_pixels.size(), sub_pixels.size());
                                for (std::size_t i = 0; i < count; ++i) {
                                    acc_pixels[i] += sub_pixels[i];
                                }
                            }
                        }
                    }
                }

                if (accumulated) {
                    auto& acc_pixels = accumulated->mutable_pixels();
                    const float inv_samples = 1.0f / static_cast<float>(samples);
                    for (float& p : acc_pixels) {
                        p *= inv_samples;
                    }
                    result.frame = std::shared_ptr<renderer2d::Framebuffer>(std::move(accumulated));
                }
            } else {
                renderer2d::DrawListBuilder builder;
                const renderer2d::DrawList2D draw_list = builder.build(*cached_comp);
                result.draw_command_count = draw_list.commands.size();

                RasterizedFrame2D rasterized = render_evaluated_composition_2d(*cached_comp, plan, task, context.renderer2d);
                if (rasterized.surface) {
                    result.frame = std::make_shared<renderer2d::Framebuffer>(std::move(*rasterized.surface));
                }
            }
        }
    }

    if (!result.frame || result.frame->width() == 0U || result.frame->height() == 0U) {
        result.frame = std::make_shared<renderer2d::Framebuffer>(
            static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.width)),
            static_cast<std::uint32_t>(std::max<std::int64_t>(1, plan.composition.height)));
    }

    // Reset tracker for future calls
    context.diagnostic_tracker = nullptr;

    return result;
}

void FrameExecutor::evaluate_node(
    std::uint32_t node_id,
    const CompiledScene& scene,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t composition_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task,
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    const CompiledNode* node = nullptr;
    auto it = m_node_lookup.find(node_id);
    if (it != m_node_lookup.end()) {
        node = it->second;
    }
    if (node == nullptr) {
        return;
    }

    const std::uint64_t node_key = build_node_key(frame_key, *node);
    switch (node->type) {
        case CompiledNodeType::Property: {
            auto prop_it = m_property_lookup.find(node_id);
            if (prop_it != m_property_lookup.end()) {
                evaluate_property(scene, *prop_it->second, plan, snapshot, context, node_key, frame_time_seconds);
            }
            break;
        }
        case CompiledNodeType::Layer: {
            auto layer_it = m_layer_lookup.find(node_id);
            if (layer_it != m_layer_lookup.end()) {
                evaluate_layer(scene, *layer_it->second, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, main_frame_key, main_frame_time);
            }
            break;
        }
        case CompiledNodeType::Composition: {
            auto comp_it = m_composition_lookup.find(node_id);
            if (comp_it != m_composition_lookup.end()) {
                evaluate_composition(scene, *comp_it->second, plan, snapshot, context, composition_key, node_key, frame_key, frame_time_seconds, task);
            }
            break;
        }
        default:
            break;
    }
}

void FrameExecutor::evaluate_property(
    const CompiledScene& scene,
    const CompiledPropertyTrack& track,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t node_key,
    double frame_time_seconds) {

    // Construct a time-dependent property key to support animation and sub-frame samples
    CacheKeyBuilder prop_builder;
    prop_builder.add_u64(node_key);
    prop_builder.add_f64(frame_time_seconds);
    const std::uint64_t prop_cache_key = prop_builder.finish();

    double cached_value = 0.0;
    if (m_cache.lookup_property(prop_cache_key, cached_value)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->property_misses++;
        context.diagnostic_tracker->properties_evaluated++;
    }

    double value = track.constant_value;
    if (track.binding.has_value()) {
        const auto it = snapshot.computed_properties.find(track.binding->data_source_index);
        if (it != snapshot.computed_properties.end()) {
            value = it->second.value;
        }
    } else if (track.kind == CompiledPropertyTrack::Kind::Keyframed) {
        value = sample_keyframed_value(track, value, frame_time_seconds);
    }

    (void)scene;
    (void)plan;
    m_cache.store_property(prop_cache_key, value);
}

void FrameExecutor::evaluate_layer(
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

    // If we are in a sub-frame but this layer has motion blur disabled, 
    // we should use the state from the main frame evaluation.
    if (is_sub_frame && !layer_motion_blur) {
        const std::uint64_t main_layer_key = build_node_key(*main_frame_key, layer.node);
        if (auto cached_main = m_cache.lookup_layer(main_layer_key)) {
            // We store the same pointer for the sub-frame sample key to avoid re-evaluation
            m_cache.store_layer(build_node_key(frame_key, layer.node), cached_main);
            return;
        }
    }

    const std::uint64_t node_key = build_node_key(frame_key, layer.node);
    if (m_cache.lookup_layer(node_key)) {
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
    state->font_size = layer.font_size;
    state->text_alignment = layer.text_alignment;
    state->text_highlights = layer.text_highlights;
    state->subtitle_path = layer.subtitle_path;
    state->subtitle_outline_color = layer.subtitle_outline_color;
    state->subtitle_outline_width = layer.subtitle_outline_width;
    state->shape_path = to_evaluated_shape_path(layer.shape_path);
    state->effects = layer.effects;
    state->precomp_id = layer.precomp_index.has_value() ? std::make_optional(std::to_string(*layer.precomp_index)) : std::nullopt;
    state->track_matte_type = layer.matte_type;
    state->track_matte_layer_index = layer.matte_layer_index.has_value()
        ? std::make_optional(static_cast<std::size_t>(*layer.matte_layer_index))
        : std::nullopt;

    if (layer.precomp_index.has_value() && *layer.precomp_index < scene.compositions.size()) {
        const auto& nested_comp = scene.compositions[*layer.precomp_index];
        const std::uint64_t nested_key = build_node_key(frame_key, nested_comp.node);
        if (auto cached_nested = m_cache.lookup_composition(nested_key)) {
            state->nested_composition = std::make_unique<scene::EvaluatedCompositionState>(*cached_nested);
        }
    }

    auto sample_property = [&](std::size_t index, double fallback) -> double {
        if (index >= layer.property_indices.size()) return fallback;
        const auto& track = scene.property_tracks[layer.property_indices[index]];
        
        // Properties must be evaluated with frame_time_seconds to handle animation correctly
        const std::uint64_t prop_node_key = build_node_key(frame_key, track.node);
        
        CacheKeyBuilder prop_builder;
        prop_builder.add_u64(prop_node_key);
        prop_builder.add_f64(frame_time_seconds);
        const std::uint64_t prop_cache_key = prop_builder.finish();

        double sampled = fallback;
        if (m_cache.lookup_property(prop_cache_key, sampled)) {
            if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
            return sampled;
        }
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_misses++;
        return fallback;
    };

    state->opacity = sample_property(0, 1.0);
    state->local_transform.position.x = static_cast<float>(sample_property(1, 0.0));
    state->local_transform.position.y = static_cast<float>(sample_property(2, 0.0));
    state->local_transform.scale.x = static_cast<float>(sample_property(3, 1.0));
    state->local_transform.scale.y = static_cast<float>(sample_property(4, 1.0));
    state->local_transform.rotation_rad = static_cast<float>(sample_property(5, 0.0) * (kPi / 180.0f));

    state->active = state->enabled && state->visible && state->opacity > 0.0;

    m_cache.store_layer(node_key, std::move(state));
}

void FrameExecutor::evaluate_composition(
    const CompiledScene& scene,
    const CompiledComposition& comp,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const std::uint64_t composition_key,
    std::uint64_t node_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task) {

    (void)composition_key;

    (void)scene;
    (void)plan;
    (void)snapshot;
    (void)context;

    if (m_cache.lookup_composition(node_key)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->composition_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->composition_misses++;
        context.diagnostic_tracker->compositions_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->width = comp.width;
    state->height = comp.height;
    state->frame_rate.numerator = comp.fps;
    state->frame_rate.denominator = 1;
    state->frame_number = task.frame_number;
    state->composition_time_seconds = frame_time_seconds;

    state->layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        const std::uint64_t layer_key = build_node_key(frame_key, layer.node); // Use frame_key (the top-level context) as base for consistency
        auto cached_layer = m_cache.lookup_layer(layer_key);
        if (cached_layer) {
            state->layers.push_back(*cached_layer);
        }
    }

    m_cache.store_composition(node_key, std::move(state));
}

ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) {

    (void)scene;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return executor.execute(compiled_scene, plan, task, *snapshot, context);
}

EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    EvaluatedFrameState state;
    state.task = task;
    state.scene_hash = compiled_scene.scene_hash;
    if (!scene.compositions.empty()) {
        const auto evaluated = scene::evaluate_scene_composition_state(
            scene,
            plan.composition_target,
            task.frame_number);
        if (evaluated.has_value()) {
            state.composition_state = std::move(*evaluated);
        }
    }
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    renderer2d::DrawListBuilder builder;
    return builder.build(state.composition_state);
}

} // namespace tachyon
