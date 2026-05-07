#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/transition_registry.h"

#include <ostream>
#include <string>
#include <vector>

namespace tachyon::analysis {

enum class InspectionSeverity {
    Info,
    Warning,
    Error,
};

struct InspectionIssue {
    InspectionSeverity severity{InspectionSeverity::Info};
    std::string code;
    std::string path;
    std::string message;
    double time{0.0};
};

struct InspectionOptions {
    int samples{5};
    bool include_info{false};
    double safe_area_ratio{0.90};
    double min_text_px{18.0};
    double min_layer_px{4.0};
};

struct InspectionReport {
    std::string schema_version{"1.0"};
    std::vector<InspectionIssue> issues;

    [[nodiscard]] bool ok() const {
        for (const auto& issue : issues) {
            if (issue.severity == InspectionSeverity::Error) {
                return false;
            }
        }
        return true;
    }
};

InspectionReport inspect_scene(
    const SceneSpec& scene,
    const TransitionRegistry& registry,
    const InspectionOptions& options = {}
);

void print_inspection_report_text(
    const InspectionReport& report,
    std::ostream& out
);

void print_inspection_report_json(
    const InspectionReport& report,
    std::ostream& out
);

} // namespace tachyon::analysis
