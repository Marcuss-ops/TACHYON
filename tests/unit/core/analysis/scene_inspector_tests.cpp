#include "tachyon/core/analysis/scene_inspector.h"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace {

bool has_issue(const tachyon::analysis::InspectionReport& report, std::string_view code) {
    return std::any_of(report.issues.begin(), report.issues.end(), [&](const auto& issue) {
        return issue.code == code;
    });
}

bool has_issue_with_severity(
    const tachyon::analysis::InspectionReport& report,
    std::string_view code,
    tachyon::analysis::InspectionSeverity severity) {
    return std::any_of(report.issues.begin(), report.issues.end(), [&](const auto& issue) {
        return issue.code == code && issue.severity == severity;
    });
}

} // namespace

bool run_scene_inspector_tests() {
    using namespace tachyon;
    using namespace tachyon::analysis;

    bool ok = true;

    {
        SceneSpec scene;
        const auto report = inspect_scene(scene);
        if (report.ok() || !has_issue(report, "scene.no_compositions")) {
            std::cerr << "scene_inspector: expected missing composition error\n";
            ok = false;
        }
    }

    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "main";
        comp.duration = -1.0;
        comp.frame_rate = FrameRate{30, 1};
        comp.audio_tracks.clear();
        LayerSpec text_layer;
        text_layer.id = "title";
        text_layer.type = LayerType::Text;
        text_layer.text_content.clear();
        text_layer.start_time = 0.0;
        text_layer.out_point = 1.0;
        comp.layers.push_back(text_layer);

        LayerSpec media_layer;
        media_layer.id = "image";
        media_layer.type = LayerType::Image;
        media_layer.start_time = 0.0;
        media_layer.out_point = 1.0;
        comp.layers.push_back(media_layer);

        scene.compositions.push_back(comp);

        const auto report = inspect_scene(scene);
        if (!has_issue(report, "composition.invalid_duration")
            || !has_issue(report, "text.empty")
            || !has_issue(report, "media.missing_asset")) {
            std::cerr << "scene_inspector: expected composition/text/media issues\n";
            ok = false;
        }
    }

    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "info";
        comp.duration = 2.0;
        comp.frame_rate = FrameRate{30, 1};
        LayerSpec text_layer;
        text_layer.id = "label";
        text_layer.type = LayerType::Text;
        text_layer.text_content = "Hello";
        text_layer.transition_in.kind = TransitionKind::Fade;
        comp.layers.push_back(text_layer);
        scene.compositions.push_back(comp);

        const auto report = inspect_scene(scene);
        if (has_issue_with_severity(report, "text.font_size_default", InspectionSeverity::Info)) {
            std::cerr << "scene_inspector: info issues should be hidden by default\n";
            ok = false;
        }

        InspectionOptions info_options;
        info_options.include_info = true;
        const auto info_report = inspect_scene(scene, info_options);
        if (!has_issue_with_severity(info_report, "layer.transition_in", InspectionSeverity::Info)) {
            std::cerr << "scene_inspector: expected info output when enabled\n";
            ok = false;
        }
    }

    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "legacy";
        comp.duration = 2.0;
        comp.frame_rate = FrameRate{30, 1};
        LayerSpec text_layer;
        text_layer.id = "title";
        text_layer.type = LayerType::Text;
        text_layer.text_content = "Hello";
        text_layer.in_preset = "tachyon.textanim.fade_in";
        text_layer.out_preset = "tachyon.textanim.fade_out";
        comp.layers.push_back(text_layer);
        scene.compositions.push_back(comp);

        const auto report = inspect_scene(scene);
        if (!has_issue_with_severity(report, "layer.in_preset", InspectionSeverity::Warning)
            || !has_issue_with_severity(report, "layer.out_preset", InspectionSeverity::Warning)) {
            std::cerr << "scene_inspector: expected warnings for legacy animation presets\n";
            ok = false;
        }
    }

    return ok;
}
