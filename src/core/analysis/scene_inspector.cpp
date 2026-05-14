#include "tachyon/core/analysis/scene_inspector.h"
#include <algorithm>
#include <set>

namespace tachyon::analysis {
namespace {

void add_issue(InspectionReport& report, InspectionSeverity severity, std::string code, std::string path, std::string message) {
    InspectionIssue issue;
    issue.severity = severity;
    issue.code = std::move(code);
    issue.path = std::move(path);
    issue.message = std::move(message);
    report.issues.push_back(std::move(issue));
}

} // namespace

InspectionReport inspect_scene(const SceneSpec& scene, const TransitionRegistry& registry, const InspectionOptions& options) {
    (void)options;
    InspectionReport report;
    std::set<std::string> comp_ids;
    for (const auto& comp : scene.compositions) {
        comp_ids.insert(comp.id);
    }

    if (scene.compositions.empty()) {
        add_issue(report, InspectionSeverity::Error, "scene.no_compositions", "scene", "Scene has no compositions.");
    }

    for (std::size_t comp_idx = 0; comp_idx < scene.compositions.size(); ++comp_idx) {
        const auto& comp = scene.compositions[comp_idx];
        std::string comp_path = "compositions[" + std::to_string(comp_idx) + "]";

        if (comp.id.empty()) {
            add_issue(report, InspectionSeverity::Error, "comp.missing_id", comp_path, "Composition ID is empty.");
        }

        if (comp.duration <= 0) {
            add_issue(report, InspectionSeverity::Warning, "comp.invalid_duration", comp_path, "Composition duration is zero or negative.");
        }

        std::set<std::string> layer_ids;
        for (std::size_t layer_idx = 0; layer_idx < comp.layers.size(); ++layer_idx) {
            const auto& layer = comp.layers[layer_idx];
            std::string layer_path = comp_path + ".layers[" + std::to_string(layer_idx) + "]";

            if (layer.identity.id.empty()) {
                add_issue(report, InspectionSeverity::Error, "layer.missing_id", layer_path, "Layer ID is empty.");
            } else if (layer_ids.count(layer.identity.id)) {
                add_issue(report, InspectionSeverity::Error, "layer.duplicate_id", layer_path, "Duplicate layer ID: " + layer.identity.id);
            }
            layer_ids.insert(layer.identity.id);

            // Hierarchy checks
            if (layer.parent.has_value()) {
                if (*layer.parent == layer.identity.id) {
                    add_issue(report, InspectionSeverity::Error, "layer.circular_parent", layer_path, "Layer cannot be its own parent.");
                }
                // Check if parent exists in the same composition
                auto it = std::find_if(comp.layers.begin(), comp.layers.end(), [&](const LayerSpec& l) {
                    return l.identity.id == *layer.parent;
                });
                if (it == comp.layers.end()) {
                    add_issue(report, InspectionSeverity::Error, "layer.missing_parent", layer_path, "Parent layer '" + *layer.parent + "' not found in composition.");
                }
            }

            // Asset checks
            if (layer.identity.type == LayerType::Image || layer.identity.type == LayerType::Video) {
                if (auto* media = std::get_if<MediaSource>(&layer.source)) {
                    if (media->asset_path.empty()) {
                        add_issue(report, InspectionSeverity::Error, "layer.missing_asset", layer_path, "Image/Video layer missing asset_path.");
                    } else {
                        auto asset_it = std::find_if(scene.assets.begin(), scene.assets.end(), [&](const AssetSpec& a) {
                            return a.id == media->asset_path;
                        });
                        if (asset_it == scene.assets.end()) {
                            add_issue(report, InspectionSeverity::Error, "layer.unresolved_asset", layer_path, "Asset '" + media->asset_path + "' not found in scene assets.");
                        }
                    }
                }
            } else if (layer.identity.type == LayerType::Precomp) {
                if (auto* precomp = std::get_if<PrecompSource>(&layer.source)) {
                    if (precomp->precomp_id.empty()) {
                        add_issue(report, InspectionSeverity::Error, "layer.missing_precomp", layer_path, "Precomp layer missing precomp_id.");
                    } else if (!comp_ids.count(precomp->precomp_id)) {
                        add_issue(report, InspectionSeverity::Error, "layer.unresolved_precomp", layer_path, "Precomp composition '" + precomp->precomp_id + "' not found.");
                    }
                }
            }

            // Transition checks (Directly in LayerSpec)
            if (!layer.transition_in.transition_id.empty() && layer.transition_in.transition_id != "none") {
                if (registry.resolve(layer.transition_in.transition_id) == nullptr) {
                    add_issue(report, InspectionSeverity::Error, "layer.transition_in.missing_id", layer_path, "Transition ID '" + layer.transition_in.transition_id + "' not found.");
                }
            }
            if (!layer.transition_out.transition_id.empty() && layer.transition_out.transition_id != "none") {
                if (registry.resolve(layer.transition_out.transition_id) == nullptr) {
                    add_issue(report, InspectionSeverity::Error, "layer.transition_out.missing_id", layer_path, "Transition ID '" + layer.transition_out.transition_id + "' not found.");
                }
            }

            // Text checks
            if (layer.identity.type == LayerType::Text) {
                if (layer.text.font_id.empty()) {
                    add_issue(report, InspectionSeverity::Warning, "layer.text.missing_font", layer_path, "Text layer missing font_id.");
                }
                if (layer.text.content.empty()) {
                    add_issue(report, InspectionSeverity::Error, "text.empty", layer_path, "Text layer has no content.");
                }
            }
        }
    }

    return report;
}

void print_inspection_report_text(const InspectionReport& report, std::ostream& out) {
    if (report.issues.empty()) {
        out << "[OK] No issues found.\n";
        return;
    }

    int errors = 0;
    int warnings = 0;
    for (const auto& issue : report.issues) {
        if (issue.severity == InspectionSeverity::Error) errors++;
        else if (issue.severity == InspectionSeverity::Warning) warnings++;

        std::string sev_str = "INFO";
        if (issue.severity == InspectionSeverity::Error) sev_str = "ERROR";
        else if (issue.severity == InspectionSeverity::Warning) sev_str = "WARN";

        out << "[" << sev_str << "] " << issue.code << " at " << issue.path << ": " << issue.message << "\n";
    }

    out << "\nSummary: " << errors << " errors, " << warnings << " warnings.\n";
}

void print_inspection_report_json(const InspectionReport& report, std::ostream& out) {
    out << "{\n  \"schema_version\": \"" << report.schema_version << "\",\n  \"issues\": [\n";
    for (size_t i = 0; i < report.issues.size(); ++i) {
        const auto& issue = report.issues[i];
        out << "    {\n";
        out << "      \"severity\": " << static_cast<int>(issue.severity) << ",\n";
        out << "      \"code\": \"" << issue.code << "\",\n";
        out << "      \"path\": \"" << issue.path << "\",\n";
        out << "      \"message\": \"" << issue.message << "\"\n";
        out << "    }" << (i == report.issues.size() - 1 ? "" : ",") << "\n";
    }
    out << "  ]\n}\n";
}

} // namespace tachyon::analysis
