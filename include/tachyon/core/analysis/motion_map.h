#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace tachyon::analysis {

struct MotionLayerSummary {
    std::string id;
    std::string type;
    double start{0.0};
    double end{0.0};
    std::vector<std::string> animations;
    std::vector<std::string> warnings;
};

struct MotionCompositionSummary {
    std::string id;
    double duration{0.0};
    double fps{0.0};
    std::vector<MotionLayerSummary> layers;
    struct RuntimeSample {
        std::int64_t frame_number{0};
        double time_seconds{0.0};
        std::size_t active_layers{0};
        std::size_t visible_layers{0};
        bool camera_available{false};
        std::string camera_type;
        std::vector<std::string> active_layer_ids;
    };
    std::vector<RuntimeSample> runtime_samples;
};

struct MotionMapReport {
    std::string schema_version{"1.0"};
    std::vector<MotionCompositionSummary> compositions;
};

struct MotionMapOptions {
    int runtime_samples{0};
};

MotionMapReport build_motion_map(const SceneSpec& scene);
MotionMapReport build_motion_map(const SceneSpec& scene, const MotionMapOptions& options);

void print_motion_map_text(const MotionMapReport& report, std::ostream& out);
void print_motion_map_json(const MotionMapReport& report, std::ostream& out);

} // namespace tachyon::analysis
