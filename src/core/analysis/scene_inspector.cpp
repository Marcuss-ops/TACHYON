#include "tachyon/core/analysis/scene_inspector.h"
#include "tachyon/transition_registry.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

namespace tachyon::analysis {
namespace {

void add_issue(
    InspectionReport& report,
    InspectionSeverity severity,
    std::string code,
    std::string path,
    std::string message,
    double time = 0.0) {
    report.issues.push_back(InspectionIssue{
        severity,
        std::move(code),
        std::move(path),
        std::move(message),
        time,
    });
}

void add_info(
    InspectionReport& report,
    const InspectionOptions& options,
    std::string code,
    std::string path,
    std::string message,
    double time = 0.0) {
    if (!options.include_info) {
        return;
    }

    add_issue(report, InspectionSeverity::Info, std::move(code), std::move(path), std::move(message), time);
}

std::string severity_to_string(InspectionSeverity severity) {
    switch (severity) {
        case InspectionSeverity::Info: return "info";
        case InspectionSeverity::Warning: return "warning";
        case InspectionSeverity::Error: return "error";
    }
    return "info";
}

std::string type_to_string(LayerType type) {
    switch (type) {
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

bool ranges_overlap(double a0, double a1, double b0, double b1) {
    return a0 < b1 && b0 < a1;
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

bool has_motion(const AnimatedScalarSpec& spec) {
    return !spec.empty();
}

bool has_motion(const AnimatedColorSpec& spec) {
    return !spec.empty();
}

bool has_motion(const AnimatedVector2Spec& spec) {
    return !spec.empty();
}

bool has_motion(const AnimatedVector3Spec& spec) {
    return !spec.empty();
}

bool has_text_motion(const TextAnimatorSpec& animator) {
    const auto& p = animator.properties;
    return p.opacity_value.has_value() || !p.opacity_keyframes.empty()
        || p.position_offset_value.has_value() || !p.position_offset_keyframes.empty()
        || p.scale_value.has_value() || !p.scale_keyframes.empty()
        || p.rotation_value.has_value() || !p.rotation_keyframes.empty()
        || p.tracking_amount_value.has_value() || !p.tracking_amount_keyframes.empty()
        || p.fill_color_value.has_value() || !p.fill_color_keyframes.empty()
        || p.stroke_color_value.has_value() || !p.stroke_color_keyframes.empty()
        || p.stroke_width_value.has_value() || !p.stroke_width_keyframes.empty()
        || p.blur_radius_value.has_value() || !p.blur_radius_keyframes.empty()
        || p.reveal_value.has_value() || !p.reveal_keyframes.empty();
}

} // namespace

InspectionReport inspect_scene(const SceneSpec& scene, const TransitionRegistry& transition_registry, const InspectionOptions& options) {
    InspectionReport report;
    (void)options.samples;

    if (scene.compositions.empty()) {
        add_issue(report, InspectionSeverity::Error, "scene.no_compositions", "scene", "Scene has no compositions.");
        return report;
    }

    for (const auto& comp : scene.compositions) {
        const std::string comp_path = "composition." + comp.id;

        if (comp.width <= 0 || comp.height <= 0) {
            add_issue(report, InspectionSeverity::Error, "composition.invalid_size", comp_path, "Composition has invalid width or height.");
        }

        if (comp.duration <= 0.0) {
            add_issue(report, InspectionSeverity::Error, "composition.invalid_duration", comp_path, "Composition duration must be positive.");
        }

        const double fps = comp.fps.has_value()
            ? static_cast<double>(*comp.fps)
            : comp.frame_rate.value();
        if (fps <= 0.0) {
            add_issue(report, InspectionSeverity::Error, "composition.invalid_fps", comp_path, "Composition fps must be positive.");
        }

        if (comp.layers.empty()) {
            add_issue(report, InspectionSeverity::Warning, "composition.no_layers", comp_path, "Composition has no layers.");
        }

        if (comp.audio_tracks.empty()) {
            add_issue(report, InspectionSeverity::Warning, "composition.no_audio", comp_path, "Composition has no audio tracks.");
        }

        for (const auto& layer : comp.layers) {
            const std::string layer_path = comp_path + ".layer." + (layer.id.empty() ? "<unnamed>" : layer.id);
            const double start = layer_start_time(layer);
            const double end = layer_end_time(layer);

            if (layer.id.empty()) {
                add_issue(report, InspectionSeverity::Error, "layer.missing_id", layer_path, "Layer has an empty id.");
            }

            if (!layer.enabled || !layer.visible) {
                add_info(report, options, "layer.disabled", layer_path, "Layer is disabled or invisible.");
            }

            if (end <= start) {
                add_issue(report, InspectionSeverity::Error, "layer.invalid_time_range", layer_path, "Layer out point must be greater than in point.");
            }

            if (start < 0.0) {
                add_issue(report, InspectionSeverity::Warning, "layer.starts_before_zero", layer_path, "Layer starts before composition time zero.", start);
            }

            if (end > comp.duration) {
                add_issue(report, InspectionSeverity::Warning, "layer.exceeds_composition", layer_path, "Layer extends beyond composition duration.", end);
            }

            if (layer.opacity <= 0.0) {
                add_issue(report, InspectionSeverity::Warning, "layer.invisible_opacity", layer_path, "Layer opacity is zero or negative.");
            }

            if (layer.width > 0 && layer.height > 0) {
                if (layer.width < options.min_layer_px || layer.height < options.min_layer_px) {
                    add_issue(report, InspectionSeverity::Warning, "layer.too_small", layer_path, "Layer is very small and may be invisible.");
                }
            }

            if (layer.type == LayerType::Text) {
                if (layer.text_content.empty()) {
                    add_issue(report, InspectionSeverity::Warning, "text.empty", layer_path, "Text layer has empty content.");
                }
                if (layer.font_size.empty()) {
                    add_info(report, options, "text.font_size_default", layer_path, "Text layer uses the default font size.");
                } else if (layer.font_size.value.has_value() && *layer.font_size.value < options.min_text_px) {
                    add_issue(report, InspectionSeverity::Warning, "text.font_too_small", layer_path, "Text layer font size is very small.");
                }
            }

            if (layer.type == LayerType::Image || layer.type == LayerType::Video) {
                if (layer.asset_id.empty()) {
                    add_issue(report, InspectionSeverity::Error, "media.missing_asset", layer_path, "Media layer has no asset_id.");
                }
            }

            if (layer.type == LayerType::Precomp && (!layer.precomp_id.has_value() || layer.precomp_id->empty())) {
                add_issue(report, InspectionSeverity::Warning, "precomp.missing_id", layer_path, "Precomp layer has no precomp_id.");
            }

            if (layer.type == LayerType::Camera && layer.camera_type.empty()) {
                add_issue(report, InspectionSeverity::Warning, "camera.missing_type", layer_path, "Camera layer has no camera type.");
            }

            if (comp.width > 0 && comp.height > 0 && layer.width > 0 && layer.height > 0) {
                const double safe_w = static_cast<double>(comp.width) * options.safe_area_ratio;
                const double safe_h = static_cast<double>(comp.height) * options.safe_area_ratio;
                if (layer.width > safe_w || layer.height > safe_h) {
                    add_issue(report, InspectionSeverity::Warning, "layer.exceeds_safe_area", layer_path, "Layer is larger than the configured safe area.");
                }
            }

            if (has_motion(layer.opacity_property)) {
                add_info(report, options, "layer.opacity_animated", layer_path, "Opacity is animated.");
            }
            if (has_motion(layer.time_remap_property)) {
                add_info(report, options, "layer.time_remap", layer_path, "Time remapping is enabled.");
            }
            if (has_motion(layer.font_size)) {
                add_info(report, options, "text.font_size_animated", layer_path, "Font size is animated.");
            }
            if (has_motion(layer.stroke_width_property)) {
                add_info(report, options, "text.stroke_width_animated", layer_path, "Stroke width is animated.");
            }
            if (has_motion(layer.light_intensity)) {
                add_info(report, options, "light.intensity_animated", layer_path, "Light intensity is animated.");
            }
            if (has_motion(layer.camera_zoom)) {
                add_info(report, options, "camera.zoom_animated", layer_path, "Camera zoom is animated.");
            }
            if (has_motion(layer.camera_poi)) {
                add_info(report, options, "camera.poi_animated", layer_path, "Camera point of interest is animated.");
            }
            if (has_motion(layer.camera_shake_amplitude_pos) || has_motion(layer.camera_shake_amplitude_rot)) {
                add_info(report, options, "camera.shake_animated", layer_path, "Camera shake is animated.");
            }
            if (has_motion(layer.repeater_count) || has_motion(layer.repeater_stagger_delay) || has_motion(layer.repeater_offset_position_x)
                || has_motion(layer.repeater_offset_position_y) || has_motion(layer.repeater_offset_rotation) || has_motion(layer.repeater_offset_scale_x)
                || has_motion(layer.repeater_offset_scale_y) || has_motion(layer.repeater_start_opacity) || has_motion(layer.repeater_end_opacity)
                || has_motion(layer.repeater_grid_cols) || has_motion(layer.repeater_radial_radius) || has_motion(layer.repeater_radial_start_angle)
                || has_motion(layer.repeater_radial_end_angle)) {
                add_info(report, options, "layer.repeater_animated", layer_path, "Repeater settings are animated.");
            }

            if (!layer.animation_in_preset.empty()) {
                add_info(report, options, "layer.animation_in_preset", layer_path, "Layer uses in preset: " + layer.animation_in_preset);
            }
            if (!layer.animation_during_preset.empty()) {
                add_info(report, options, "layer.animation_during_preset", layer_path, "Layer uses during preset: " + layer.animation_during_preset);
            }
            if (!layer.animation_out_preset.empty()) {
                add_info(report, options, "layer.animation_out_preset", layer_path, "Layer uses out preset: " + layer.animation_out_preset);
            }

            if (layer.transition_in.kind != TransitionKind::None || !layer.transition_in.transition_id.empty()) {
                add_info(report, options, "layer.transition_in", layer_path, "Layer has an entrance transition.");
                if (!layer.transition_in.transition_id.empty()) {
                    if (transition_registry.resolve(layer.transition_in.transition_id) == nullptr) {
                        add_issue(report, InspectionSeverity::Error, "layer.transition_in.missing_id", layer_path,
                            "Transition ID '" + layer.transition_in.transition_id + "' not found in registry.");
                    }
                }
            }
            if (layer.transition_out.kind != TransitionKind::None || !layer.transition_out.transition_id.empty()) {
                add_info(report, options, "layer.transition_out", layer_path, "Layer has an exit transition.");
                if (!layer.transition_out.transition_id.empty()) {
                    if (transition_registry.resolve(layer.transition_out.transition_id) == nullptr) {
                        add_issue(report, InspectionSeverity::Error, "layer.transition_out.missing_id", layer_path,
                            "Transition ID '" + layer.transition_out.transition_id + "' not found in registry.");
                    }
                }
            }

            if (layer.loop) {
                add_issue(report, InspectionSeverity::Warning, "layer.loop_without_duration", layer_path, "Layer loops and may need an explicit duration.");
            }

            if (!layer.text_animators.empty()) {
                add_info(report, options, "text.animators", layer_path, "Layer has " + std::to_string(layer.text_animators.size()) + " text animator(s).");
                for (const auto& animator : layer.text_animators) {
                    if (has_text_motion(animator)) {
                        add_info(report, options, "text.animator_motion", layer_path, "Text animator '" + animator.name + "' is animated.");
                    }
                }
            }

            if (!layer.markers.empty()) {
                add_info(report, options, "layer.markers", layer_path, "Layer has markers.");
            }

            if (layer.procedural.has_value()) {
                add_info(report, options, "layer.procedural", layer_path, "Layer uses procedural generation.");
            }

            if (layer.particle_spec.has_value()) {
                add_info(report, options, "layer.particles", layer_path, "Layer uses a particle spec.");
            }
        }

        for (std::size_t i = 0; i < comp.layers.size(); ++i) {
            for (std::size_t j = i + 1; j < comp.layers.size(); ++j) {
                const auto& a = comp.layers[i];
                const auto& b = comp.layers[j];
                if (!a.enabled || !a.visible || !b.enabled || !b.visible) {
                    continue;
                }

                if (ranges_overlap(layer_start_time(a), layer_end_time(a), layer_start_time(b), layer_end_time(b))) {
                    if (a.type == b.type || a.type == LayerType::Text || b.type == LayerType::Text) {
                        const std::string a_id = a.id.empty() ? "<unnamed>" : a.id;
                        const std::string b_id = b.id.empty() ? "<unnamed>" : b.id;
                        add_issue(report, InspectionSeverity::Warning, "layer.overlap_time", comp_path,
                            "Layers overlap in time: " + a_id + " and " + b_id);
                    }
                }
            }
        }
    }

    return report;
}

void print_inspection_report_text(const InspectionReport& report, std::ostream& out) {
    std::size_t info_count = 0;
    std::size_t warning_count = 0;
    std::size_t error_count = 0;
    for (const auto& issue : report.issues) {
        switch (issue.severity) {
            case InspectionSeverity::Info: ++info_count; break;
            case InspectionSeverity::Warning: ++warning_count; break;
            case InspectionSeverity::Error: ++error_count; break;
        }
    }

    out << "scene inspection\n";
    out << "  schema version: " << report.schema_version << '\n';
    out << "  issues: " << error_count << " errors, " << warning_count << " warnings, " << info_count << " info\n";

    if (report.issues.empty()) {
        out << "  no issues found\n";
        return;
    }

    for (const auto& issue : report.issues) {
        out << '[' << severity_to_string(issue.severity) << "] " << issue.code << "  " << issue.path << ": " << issue.message;
        if (issue.time != 0.0) {
            out << " (t=" << std::fixed << std::setprecision(3) << issue.time << ")";
        }
        out << '\n';
    }
}

static std::string escape_json(std::string_view value) {
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

void print_inspection_report_json(const InspectionReport& report, std::ostream& out) {
    out << "{\n";
    out << "  \"schema_version\": \"" << escape_json(report.schema_version) << "\",\n";
    out << "  \"ok\": " << (report.ok() ? "true" : "false") << ",\n";
    out << "  \"issues\": [\n";
    for (std::size_t i = 0; i < report.issues.size(); ++i) {
        const auto& issue = report.issues[i];
        out << "    {\n";
        out << "      \"severity\": \"" << severity_to_string(issue.severity) << "\",\n";
        out << "      \"code\": \"" << escape_json(issue.code) << "\",\n";
        out << "      \"path\": \"" << escape_json(issue.path) << "\",\n";
        out << "      \"message\": \"" << escape_json(issue.message) << "\",\n";
        out << "      \"time\": " << issue.time << "\n";
        out << "    }" << (i + 1 < report.issues.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

} // namespace tachyon::analysis
