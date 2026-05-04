#include "tachyon/core/analysis/motion_map.h"
#include "tachyon/timeline/evaluator.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace tachyon::analysis {
namespace {

std::string layer_kind_to_string(LayerType kind) {
    switch (kind) {
        case LayerType::Solid: return "solid";
        case LayerType::Shape: return "shape";
        case LayerType::Image: return "image";
        case LayerType::Video: return "video";
        case LayerType::Text: return "text";
        case LayerType::Camera: return "camera";
        case LayerType::Precomp: return "precomp";
        case LayerType::Light: return "light";
        case LayerType::Mask: return "mask";
        case LayerType::NullLayer: return "null";
        case LayerType::Procedural: return "procedural";
        case LayerType::Unknown: return "unknown";
    }
    return "unknown";
}

std::string transition_kind_to_string(TransitionKind kind) {
    switch (kind) {
        case TransitionKind::None: return "none";
        case TransitionKind::Fade: return "fade";
        case TransitionKind::Slide: return "slide";
        case TransitionKind::Zoom: return "zoom";
        case TransitionKind::Flip: return "flip";
        case TransitionKind::Wipe: return "wipe";
        case TransitionKind::Dissolve: return "dissolve";
        case TransitionKind::Custom: return "custom";
    }
    return "none";
}

template <typename T>
bool has_motion(const T& value) {
    return !value.empty();
}

void append_unique(std::vector<std::string>& list, std::string value) {
    if (std::find(list.begin(), list.end(), value) == list.end()) {
        list.push_back(std::move(value));
    }
}

double layer_start_time(const LayerSpec& layer) {
    return layer.start_time;
}

double layer_end_time(const LayerSpec& layer) {
    if (layer.duration.has_value()) {
        return layer.start_time + *layer.duration;
    }
    return layer.out_point;
}

std::vector<std::string> summarize_text_motion(const TextAnimatorSpec& animator) {
    std::vector<std::string> hints;
    if (animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty()) {
        hints.push_back("opacity");
    }
    if (animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty()) {
        hints.push_back("position");
    }
    if (animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty()) {
        hints.push_back("scale");
    }
    if (animator.properties.rotation_value.has_value() || !animator.properties.rotation_keyframes.empty()) {
        hints.push_back("rotation");
    }
    if (animator.properties.tracking_amount_value.has_value() || !animator.properties.tracking_amount_keyframes.empty()) {
        hints.push_back("tracking");
    }
    if (animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty()) {
        hints.push_back("fill_color");
    }
    if (animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty()) {
        hints.push_back("stroke_color");
    }
    if (animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty()) {
        hints.push_back("stroke_width");
    }
    if (animator.properties.blur_radius_value.has_value() || !animator.properties.blur_radius_keyframes.empty()) {
        hints.push_back("blur");
    }
    if (animator.properties.reveal_value.has_value() || !animator.properties.reveal_keyframes.empty()) {
        hints.push_back("reveal");
    }
    return hints;
}

std::string escape_json(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::int64_t sample_frame_number(const CompositionSpec& comp, int sample_index, int sample_count) {
    if (sample_count <= 1) {
        return 0;
    }

    const double fps = comp.fps.has_value() ? static_cast<double>(*comp.fps) : comp.frame_rate.value();
    const double duration = comp.duration > 0.0 ? comp.duration : 0.0;
    const double max_frame = std::max(0.0, duration * std::max(1.0, fps));
    const double t = static_cast<double>(sample_index) / static_cast<double>(sample_count - 1);
    return static_cast<std::int64_t>(std::llround(max_frame * t));
}

std::vector<MotionCompositionSummary::RuntimeSample> build_runtime_samples(
    const SceneSpec& scene,
    const CompositionSpec& comp,
    int sample_count) {
    std::vector<MotionCompositionSummary::RuntimeSample> samples;
    if (sample_count <= 0) {
        return samples;
    }

    samples.reserve(static_cast<std::size_t>(sample_count));
    for (int i = 0; i < sample_count; ++i) {
        const std::int64_t frame_number = sample_frame_number(comp, i, sample_count);
        const auto state = ::tachyon::timeline::evaluate_composition_frame({
            .scene = &scene,
            .composition_id = comp.id,
            .frame_number = frame_number,
        });

        MotionCompositionSummary::RuntimeSample sample;
        sample.frame_number = frame_number;
        sample.time_seconds = comp.fps.has_value() && *comp.fps > 0
            ? static_cast<double>(frame_number) / static_cast<double>(*comp.fps)
            : static_cast<double>(frame_number) / std::max(1.0, comp.frame_rate.value());

        if (state.has_value()) {
            sample.camera_available = state->camera.available;
            sample.camera_type = state->camera.camera_type;
            sample.active_layers = state->layers.size();
            sample.visible_layers = std::count_if(
                state->layers.begin(),
                state->layers.end(),
                [](const auto& layer) { return layer.visible && layer.enabled && layer.active; });
            sample.active_layer_ids.reserve(state->layers.size());
            for (const auto& layer : state->layers) {
                if (layer.visible && layer.enabled && layer.active) {
                    sample.active_layer_ids.push_back(layer.id);
                }
            }
        }

        samples.push_back(std::move(sample));
    }

    return samples;
}

} // namespace

MotionMapReport build_motion_map(const SceneSpec& scene) {
    return build_motion_map(scene, MotionMapOptions{});
}

MotionMapReport build_motion_map(const SceneSpec& scene, const MotionMapOptions& options) {
    MotionMapReport report;
    report.compositions.reserve(scene.compositions.size());

    for (const auto& comp : scene.compositions) {
        MotionCompositionSummary comp_summary;
        comp_summary.id = comp.id;
        comp_summary.duration = comp.duration;
        comp_summary.fps = comp.fps.has_value() ? static_cast<double>(*comp.fps) : comp.frame_rate.value();
        comp_summary.layers.reserve(comp.layers.size());

        for (const auto& layer : comp.layers) {
            MotionLayerSummary layer_summary;
            layer_summary.id = layer.id;
            layer_summary.type = layer_kind_to_string(layer.kind);
            layer_summary.start = layer_start_time(layer);
            layer_summary.end = layer_end_time(layer);

            if (has_motion(layer.opacity_property)) {
                append_unique(layer_summary.animations, "opacity");
            }
            if (has_motion(layer.time_remap_property)) {
                append_unique(layer_summary.animations, "time_remap");
            }
            if (!layer.in_preset.empty()) {
                append_unique(layer_summary.animations, "in_preset:" + layer.in_preset);
            }
            if (!layer.during_preset.empty()) {
                append_unique(layer_summary.animations, "during_preset:" + layer.during_preset);
            }
            if (!layer.out_preset.empty()) {
                append_unique(layer_summary.animations, "out_preset:" + layer.out_preset);
            }
            if (layer.transition_in.kind != TransitionKind::None) {
                append_unique(layer_summary.animations, "transition_in:" + transition_kind_to_string(layer.transition_in.kind));
            }
            if (layer.transition_out.kind != TransitionKind::None) {
                append_unique(layer_summary.animations, "transition_out:" + transition_kind_to_string(layer.transition_out.kind));
            }
            if (layer.motion_blur) {
                append_unique(layer_summary.animations, "motion_blur");
            }
            if (layer.loop) {
                append_unique(layer_summary.animations, "loop");
            }
            if (layer.is_3d) {
                append_unique(layer_summary.animations, "3d_transform");
            }
            if (layer.kind == LayerType::Text && !layer.text_content.empty()) {
                append_unique(layer_summary.animations, "text_content");
            }
            if (!layer.text_animators.empty()) {
                append_unique(layer_summary.animations, "text_animators:" + std::to_string(layer.text_animators.size()));
                for (const auto& animator : layer.text_animators) {
                    const auto hints = summarize_text_motion(animator);
                    for (const auto& hint : hints) {
                        append_unique(layer_summary.animations, "text:" + hint);
                    }
                    if (!animator.name.empty()) {
                        append_unique(layer_summary.animations, "animator:" + animator.name);
                    } else {
                        append_unique(layer_summary.animations, "animator:" + animator.selector.type);
                    }
                }
            }
            if (!layer.text_highlights.empty()) {
                append_unique(layer_summary.animations, "text_highlights:" + std::to_string(layer.text_highlights.size()));
            }
            if (layer.procedural.has_value()) {
                append_unique(layer_summary.animations, "procedural");
            }
            if (layer.particle_spec.has_value()) {
                append_unique(layer_summary.animations, "particles");
            }
            if (has_motion(layer.camera_zoom)) {
                append_unique(layer_summary.animations, "camera_zoom");
            }
            if (has_motion(layer.camera_poi)) {
                append_unique(layer_summary.animations, "camera_poi");
            }
            if (has_motion(layer.light_intensity)) {
                append_unique(layer_summary.animations, "light_intensity");
            }
            if (has_motion(layer.repeater_count) || has_motion(layer.repeater_stagger_delay) || has_motion(layer.repeater_offset_position_x)
                || has_motion(layer.repeater_offset_position_y) || has_motion(layer.repeater_offset_rotation)
                || has_motion(layer.repeater_offset_scale_x) || has_motion(layer.repeater_offset_scale_y)
                || has_motion(layer.repeater_start_opacity) || has_motion(layer.repeater_end_opacity)
                || has_motion(layer.repeater_grid_cols) || has_motion(layer.repeater_radial_radius)
                || has_motion(layer.repeater_radial_start_angle) || has_motion(layer.repeater_radial_end_angle)) {
                append_unique(layer_summary.animations, "repeater");
            }

            if (!layer.enabled || !layer.visible) {
                layer_summary.warnings.push_back("disabled_or_invisible");
            }
            if (layer.kind == LayerType::Text && layer.text_content.empty()) {
                layer_summary.warnings.push_back("empty_text");
            }
            if ((layer.kind == LayerType::Image || layer.kind == LayerType::Video) && layer.asset_id.empty()) {
                layer_summary.warnings.push_back("missing_asset_id");
            }

            comp_summary.layers.push_back(std::move(layer_summary));
        }

        if (options.runtime_samples > 0) {
            comp_summary.runtime_samples = build_runtime_samples(scene, comp, options.runtime_samples);
        }

        report.compositions.push_back(std::move(comp_summary));
    }

    return report;
}

void print_motion_map_text(const MotionMapReport& report, std::ostream& out) {
    out << "motion map\n";
    out << "  schema version: " << report.schema_version << '\n';
    out << "  compositions: " << report.compositions.size() << '\n';

    for (const auto& comp : report.compositions) {
        out << "  composition " << comp.id << "  duration " << comp.duration << "s  fps " << comp.fps << '\n';
        if (!comp.runtime_samples.empty()) {
            out << "    runtime samples: " << comp.runtime_samples.size() << '\n';
            for (const auto& sample : comp.runtime_samples) {
                out << "      @ frame " << sample.frame_number << " (" << sample.time_seconds << "s)"
                    << " active=" << sample.active_layers
                    << " visible=" << sample.visible_layers
                    << " camera=" << (sample.camera_available ? sample.camera_type : "default") << '\n';
                if (!sample.active_layer_ids.empty()) {
                    out << "        active layers: ";
                    for (std::size_t i = 0; i < sample.active_layer_ids.size(); ++i) {
                        if (i > 0) {
                            out << ", ";
                        }
                        out << sample.active_layer_ids[i];
                    }
                    out << '\n';
                }
            }
        }
        for (const auto& layer : comp.layers) {
            out << "    - " << layer.id << " [" << layer.type << "] " << layer.start << " -> " << layer.end << '\n';
            if (!layer.animations.empty()) {
                out << "      animations: ";
                for (std::size_t i = 0; i < layer.animations.size(); ++i) {
                    if (i > 0) {
                        out << ", ";
                    }
                    out << layer.animations[i];
                }
                out << '\n';
            }
            if (!layer.warnings.empty()) {
                out << "      warnings: ";
                for (std::size_t i = 0; i < layer.warnings.size(); ++i) {
                    if (i > 0) {
                        out << ", ";
                    }
                    out << layer.warnings[i];
                }
                out << '\n';
            }
        }
    }
}

void print_motion_map_json(const MotionMapReport& report, std::ostream& out) {
    out << "{\n";
    out << "  \"schema_version\": \"" << escape_json(report.schema_version) << "\",\n";
    out << "  \"compositions\": [\n";
    for (std::size_t i = 0; i < report.compositions.size(); ++i) {
        const auto& comp = report.compositions[i];
        out << "    {\n";
        out << "      \"id\": \"" << escape_json(comp.id) << "\",\n";
        out << "      \"duration\": " << comp.duration << ",\n";
        out << "      \"fps\": " << comp.fps << ",\n";
        out << "      \"runtime_samples\": [\n";
        for (std::size_t s = 0; s < comp.runtime_samples.size(); ++s) {
            const auto& sample = comp.runtime_samples[s];
            out << "        {\n";
            out << "          \"frame_number\": " << sample.frame_number << ",\n";
            out << "          \"time_seconds\": " << sample.time_seconds << ",\n";
            out << "          \"active_layers\": " << sample.active_layers << ",\n";
            out << "          \"visible_layers\": " << sample.visible_layers << ",\n";
            out << "          \"camera_available\": " << (sample.camera_available ? "true" : "false") << ",\n";
            out << "          \"camera_type\": \"" << escape_json(sample.camera_type) << "\",\n";
            out << "          \"active_layer_ids\": [";
            for (std::size_t k = 0; k < sample.active_layer_ids.size(); ++k) {
                if (k > 0) {
                    out << ", ";
                }
                out << "\"" << escape_json(sample.active_layer_ids[k]) << "\"";
            }
            out << "]\n";
            out << "        }" << (s + 1 < comp.runtime_samples.size() ? "," : "") << "\n";
        }
        out << "      ],\n";
        out << "      \"layers\": [\n";
        for (std::size_t j = 0; j < comp.layers.size(); ++j) {
            const auto& layer = comp.layers[j];
            out << "        {\n";
            out << "          \"id\": \"" << escape_json(layer.id) << "\",\n";
            out << "          \"type\": \"" << escape_json(layer.type) << "\",\n";
            out << "          \"start\": " << layer.start << ",\n";
            out << "          \"end\": " << layer.end << ",\n";
            out << "          \"animations\": [";
            for (std::size_t k = 0; k < layer.animations.size(); ++k) {
                if (k > 0) {
                    out << ", ";
                }
                out << "\"" << escape_json(layer.animations[k]) << "\"";
            }
            out << "],\n";
            out << "          \"warnings\": [";
            for (std::size_t k = 0; k < layer.warnings.size(); ++k) {
                if (k > 0) {
                    out << ", ";
                }
                out << "\"" << escape_json(layer.warnings[k]) << "\"";
            }
            out << "]\n";
            out << "        }" << (j + 1 < comp.layers.size() ? "," : "") << "\n";
        }
        out << "      ]\n";
        out << "    }" << (i + 1 < report.compositions.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

} // namespace tachyon::analysis
