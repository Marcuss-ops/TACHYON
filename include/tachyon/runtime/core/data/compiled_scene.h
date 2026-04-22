/**
 * @file compiled_scene.h
 * @brief Runtime Optimized Binary Scene Representation (ABI Boundary).
 * 
 * This file defines the optimized structures used by the rendering execution 
 * engine. These structures are designed for cache locality, minimal pointer 
 * chasing, and direct batch processing.
 */

#pragma once

#include "tachyon/runtime/core/contracts/determinism_contract.h"
#include "tachyon/runtime/core/data/data_binding.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/vm/expression_vm.h"
#include "tachyon/runtime/core/graph/render_graph.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

/**
 * @brief Types of nodes present in the execution graph.
 */
enum class CompiledNodeType : std::uint8_t {
    Scene,
    Composition,
    Layer,
    Property,
    Expression,
    Track,
    Asset,
    Unknown
};

/**
 * @brief Base structure for any node in the compiled dependency graph.
 */
struct CompiledNode {
    std::uint32_t node_id{0};
    std::uint32_t version{0};
    std::uint32_t topo_index{0};
    std::uint64_t dirty_mask{0};
    CompiledNodeType type{CompiledNodeType::Unknown};
    
    std::vector<std::uint32_t> dependencies; ///< IDs of nodes this node depends on.
    std::vector<std::uint32_t> dependents;   ///< IDs of nodes that depend on this node.
};

/**
 * @brief Binary header used for scene validation and caching offsets.
 */
struct CompiledSceneHeader {
    std::uint32_t magic{0x54414348U};     ///< "TACH" magic number.
    std::uint16_t version{1};            ///< Major ABI version.
    std::uint16_t flags{0};              ///< Processing flags (reserved).
    std::uint32_t layout_version{1};     ///< Structure layout version.
    std::uint32_t compiler_version{1};   ///< The version of the compiler that produced this blob.
    std::uint64_t layout_checksum{0};    ///< Checksum of the logical structure.
};

struct CompiledKeyframe {
    double time{0.0};
    double value{0.0};
    std::uint32_t easing{0};
    double cx1{0.0};
    double cy1{0.0};
    double cx2{1.0};
    double cy2{1.0};
};

struct CompiledPropertyTrack {
    enum class Kind : std::uint8_t {
        Constant,
        Keyframed,
        Expression,
        Binding
    };

    CompiledNode node;
    std::string property_id;
    Kind kind{Kind::Constant};
    double constant_value{0.0};
    std::vector<CompiledKeyframe> keyframes;
    std::optional<std::uint32_t> expression_index;
    std::optional<DataBindingSpec> binding;
};

struct CompiledLayer {
    CompiledNode node;
    std::uint32_t type_id{0}; // Resolved enum or index
    
    // Boundary data
    std::uint32_t width{0};
    std::uint32_t height{0};

    // Render-relevant authoring data preserved for execution.
    std::string text_content;
    std::string font_id;
    float font_size{48.0f};
    int text_alignment{0};
    ColorSpec fill_color{255, 255, 255, 255};
    ColorSpec stroke_color{255, 255, 255, 255};
    float stroke_width{0.0f};
    std::optional<ShapePathSpec> shape_path;
    std::vector<EffectSpec> effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;
    float mask_feather{0.0f};
    std::optional<std::string> subtitle_path;
    std::optional<ColorSpec> subtitle_outline_color;
    float subtitle_outline_width{0.0f};
    
    // Evaluation state indices
    std::optional<std::uint32_t> parent_index;
    std::optional<std::uint32_t> precomp_index;
    std::vector<std::uint32_t> property_indices; // Indices into CompiledScene::property_tracks
    
    // Masking and Matte (resolved into indices)
    TrackMatteType matte_type{TrackMatteType::None};
    std::optional<std::uint32_t> matte_layer_index;
    
    // Style (captured for DrawList emission)
    renderer2d::LineCap line_cap{renderer2d::LineCap::Butt};
    renderer2d::LineJoin line_join{renderer2d::LineJoin::Miter};
    float miter_limit{4.0f};
    
    // Visibility flags (bitmask preferred for industrial minimality)
    std::uint8_t flags{0x01}; // 0x01 = enabled, 0x02 = visible, 0x04 = is_3d, 0x08 = adjustment
};

struct CompiledComposition {
    CompiledNode node;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t fps{60};
    double duration{0.0};
    std::vector<CompiledLayer> layers;
};

struct CompiledScene {
    CompiledSceneHeader header;
    DeterminismContract determinism;
    std::uint64_t scene_hash{0};
    std::string project_id;
    std::string project_name;
    
    RenderGraph graph;
    
    std::vector<CompiledComposition> compositions;
    std::vector<CompiledPropertyTrack> property_tracks;
    std::vector<CompiledExpression> expressions;
    std::vector<AssetSpec> assets;

    [[nodiscard]] bool is_valid() const noexcept;

    /**
     * @brief Increments the version of the specified node and all its dependents.
     */
    void invalidate_node(std::uint32_t node_id);



    /**
     * @brief Calculates a stable checksum representing the binary layout of internal structures.
     */
    [[nodiscard]] static constexpr std::uint64_t calculate_layout_checksum() noexcept {
        std::uint64_t h = 0x9e3779b97f4a7c15ULL;
        const auto mix = [&h](std::size_t s) noexcept {
            h ^= static_cast<std::uint64_t>(s) + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
        };
        
        mix(sizeof(CompiledSceneHeader));
        mix(sizeof(CompiledScene));
        mix(sizeof(CompiledComposition));
        mix(sizeof(CompiledLayer));
        mix(sizeof(CompiledPropertyTrack));
        mix(sizeof(CompiledKeyframe));
        mix(sizeof(DeterminismContract));
        mix(sizeof(CompiledNode));
        
        return h;
    }
};

} // namespace tachyon
