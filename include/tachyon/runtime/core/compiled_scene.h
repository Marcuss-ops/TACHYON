#pragma once

#include "tachyon/runtime/core/determinism_policy.h"
#include "tachyon/runtime/vm/expression_vm.h"
#include "tachyon/runtime/core/property_graph.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

struct CompiledSceneHeader {
    std::uint32_t magic{0x54414348U}; // "TACH"
    std::uint16_t version{1};
    std::uint16_t flags{0};
    std::uint32_t layout_version{1};
    std::uint32_t compiler_version{1};
};

struct CompiledKeyframe {
    double time{0.0};
    double value{0.0};
    std::uint32_t easing{0};
};

struct CompiledPropertyTrack {
    enum class Kind : std::uint8_t {
        Constant,
        Keyframed,
        Expression,
        Binding
    };

    std::string property_id;
    Kind kind{Kind::Constant};
    double constant_value{0.0};
    std::vector<CompiledKeyframe> keyframes;
    std::optional<std::uint32_t> expression_index;
    std::uint64_t dependency_mask{0};
};

struct CompiledLayer {
    std::string id;
    std::string name;
    std::string type;
    std::string blend_mode{"normal"};
    std::optional<std::uint32_t> parent_index;
    bool enabled{true};
    bool visible{true};
    bool is_3d{false};
    bool is_adjustment_layer{false};
    double start_time{0.0};
    double in_point{0.0};
    double out_point{0.0};
    double opacity{1.0};
    std::uint64_t dependency_mask{0};
    std::vector<std::uint32_t> property_indices;
};

struct CompiledComposition {
    std::string id;
    std::string name;
    std::uint32_t width{0};
    std::uint32_t height{0};
    double duration{0.0};
    std::uint32_t fps{60};
    std::vector<CompiledLayer> layers;
};

struct CompiledScene {
    CompiledSceneHeader header;
    DeterminismPolicy determinism;
    std::uint64_t scene_hash{0};
    std::string project_id;
    std::string project_name;
    std::vector<CompiledComposition> compositions;
    std::vector<CompiledPropertyTrack> property_tracks;
    PropertyGraph property_graph;
    std::vector<CompiledExpression> expressions;
};

} // namespace tachyon

