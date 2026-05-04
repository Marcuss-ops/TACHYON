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
        text_layer.kind = LayerType::Text;
        text_layer.text_content.clear();
        text_layer.start_time = 0.0;
        text_layer.out_point = 1.0;
        comp.layers.push_back(text_layer);

        LayerSpec media_layer;
        media_layer.id = "image";
        media_layer.kind = LayerType::Image;
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

    return ok;
}
