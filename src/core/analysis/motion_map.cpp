#include "tachyon/core/analysis/motion_map.h"
#include <algorithm>
#include <map>

namespace tachyon::analysis {
namespace {

void append_unique(std::vector<std::string>& vec, std::string val) {
    if (std::find(vec.begin(), vec.end(), val) == vec.end()) {
        vec.push_back(std::move(val));
    }
}

} // namespace

MotionMapReport build_motion_map(const SceneSpec& scene, const MotionMapOptions& options) {
    (void)options;
    MotionMapReport report;
    for (const auto& comp : scene.compositions) {
        MotionCompositionSummary comp_summary;
        comp_summary.id = comp.id;
        comp_summary.duration = comp.duration;
        comp_summary.fps = comp.fps.has_value() ? static_cast<double>(*comp.fps) : comp.frame_rate.value();

        if (options.runtime_samples > 0) {
            double step = (comp.duration > 0) ? comp.duration / static_cast<double>(options.runtime_samples) : 1.0;
            for (int i = 0; i < options.runtime_samples; ++i) {
                MotionCompositionSummary::RuntimeSample sample;
                sample.time_seconds = static_cast<double>(i) * step;
                sample.frame_number = static_cast<std::int64_t>(std::floor(sample.time_seconds * comp_summary.fps));
                comp_summary.runtime_samples.push_back(sample);
            }
        }

        for (const auto& layer : comp.layers) {
            MotionLayerSummary layer_summary;
            layer_summary.id = layer.identity.id;
            layer_summary.type = std::string(to_canonical_layer_type_string(layer.identity.type));
            layer_summary.start = layer.playback.timing.start;
            layer_summary.end = layer.playback.timing.start + layer.playback.timing.duration;

            // Check transitions
            if (!layer.transition_in.transition_id.empty() && layer.transition_in.transition_id != "none") {
                append_unique(layer_summary.animations, "transition_in:" + layer.transition_in.transition_id);
            }
            if (!layer.transition_out.transition_id.empty() && layer.transition_out.transition_id != "none") {
                append_unique(layer_summary.animations, "transition_out:" + layer.transition_out.transition_id);
            }



            // Check animated properties
            if (!layer.transform.opacity_property.keyframes.empty()) append_unique(layer_summary.animations, "opacity");
            if (!layer.transform.transform.position_property.keyframes.empty()) append_unique(layer_summary.animations, "position");
            if (!layer.transform.transform.rotation_property.keyframes.empty()) append_unique(layer_summary.animations, "rotation");
            if (!layer.transform.transform.scale_property.keyframes.empty()) append_unique(layer_summary.animations, "scale");

            // Text animators
            for (const auto& animator : layer.text_animators) {
                append_unique(layer_summary.animations, "text_animator:" + animator.name);
            }

            comp_summary.layers.push_back(std::move(layer_summary));
        }
        report.compositions.push_back(std::move(comp_summary));
    }
    return report;
}

MotionMapReport build_motion_map(const SceneSpec& scene) {
    return build_motion_map(scene, MotionMapOptions{});
}

void print_motion_map_text(const MotionMapReport& report, std::ostream& out) {
    out << "motion map report v" << report.schema_version << "\n";
    for (const auto& comp : report.compositions) {
        out << "composition: " << comp.id << " (" << comp.duration << "s, " << comp.fps << "fps)\n";
        for (const auto& layer : comp.layers) {
            out << "  layer: " << layer.id << " [" << layer.type << "] (" << layer.start << "s -> " << layer.end << "s)\n";
            for (const auto& anim : layer.animations) {
                out << "    - " << anim << "\n";
            }
        }
    }
}

void print_motion_map_json(const MotionMapReport& report, std::ostream& out) {
    // Basic JSON implementation
    out << "{\n  \"schema_version\": \"" << report.schema_version << "\",\n  \"compositions\": [\n";
    for (std::size_t i = 0; i < report.compositions.size(); ++i) {
        const auto& comp = report.compositions[i];
        out << "    {\n      \"id\": \"" << comp.id << "\",\n      \"duration\": " << comp.duration << ",\n      \"layers\": [\n";
        for (std::size_t j = 0; j < comp.layers.size(); ++j) {
            const auto& layer = comp.layers[j];
            out << "        {\n          \"id\": \"" << layer.id << "\",\n          \"type\": \"" << layer.type << "\",\n";
            out << "          \"start\": " << layer.start << ",\n          \"end\": " << layer.end << ",\n          \"animations\": [";
            for (std::size_t k = 0; k < layer.animations.size(); ++k) {
                out << "\"" << layer.animations[k] << "\"" << (k + 1 < layer.animations.size() ? ", " : "");
            }
            out << "]\n        }" << (j + 1 < comp.layers.size() ? "," : "") << "\n";
        }
        out << "      ]\n    }" << (i + 1 < report.compositions.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

} // namespace tachyon::analysis
