#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/effects/effect_spec.h"
#include "tachyon/core/math/transform2.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace tachyon::scene {

struct EvaluatedCompositionState;

struct EvaluatedMaskState {
    std::string id;
    std::vector<math::Vector2> points;
    float feather{0.0f};
    float opacity{1.0f};
    bool inverted{false};
};

struct EvaluatedShapePoint {
    math::Vector2 position;
    math::Vector2 tangent_in;
    math::Vector2 tangent_out;
};

struct EvaluatedShapePath {
    std::vector<EvaluatedShapePoint> points;
    bool closed{false};
};

struct EvaluatedLayerState {
    // 1. Identity & Type
    struct Identity {
        std::string layer_id;
        std::string id;
        std::string name;
        LayerType type{LayerType::NullLayer};
        bool enabled{true};
        bool active{true};
        bool visible{true};
        bool is_adjustment_layer{false};
        bool motion_blur{false};
    } identity;

    // 2. Geometry & Transform
    struct Transform {
        std::int64_t width{0};
        std::int64_t height{0};
        double opacity{1.0};
        std::string blend_mode{"normal"};
        math::Transform2 local_transform;
        math::Matrix3x3 world_matrix;
    } transform;

    // 3. Text Specific
    struct Text {
        std::string content;
        std::string font_id;
        float font_size{24.0f};
        ColorSpec fill_color{255, 255, 255, 255};
        ColorSpec stroke_color{0, 0, 0, 0};
        float stroke_width{0.0f};
        HorizontalAlign alignment{HorizontalAlign::Left};
        TextBoxSpec box; // Changed to TextBoxSpec
        std::vector<TextAnimatorSpec> animators;
        std::vector<TextHighlightSpec> highlights;
    } text;

    // 4. Source & Media
    struct Source {
        LayerSource definition;
        double start_time{0.0};
        double duration{0.0};
        bool loop{false};

        // Compatibility helpers
        std::optional<std::string> asset_path() const {
            if (const auto* m = std::get_if<MediaSource>(&definition)) return m->asset.id;
            return std::nullopt;
        }
        std::optional<std::string> precomp_id() const {
            if (const auto* p = std::get_if<PrecompSource>(&definition)) return p->precomp_id;
            return std::nullopt;
        }
        const ProceduralSpec* procedural() const {
            if (const auto* p = std::get_if<ProceduralSource>(&definition)) return &p->spec;
            return nullptr;
        }
    } source;

    // 5. Vector & Shape
    struct Vector {
        std::optional<EvaluatedShapePath> shape_path;
        spec::LineCap line_cap{spec::LineCap::Butt};
        spec::LineJoin line_join{spec::LineJoin::Miter};
        float miter_limit{4.0f};
    } vector;

    // 6. Masks
    struct Masks {
        std::vector<spec::MaskPath> list;
        bool empty() const { return list.empty(); }
    } masks;

    // 7. Subtitles
    struct Subtitles {
        bool enabled{false};
        std::string style_id;
    } subtitles;

    // 8. Playback & Timing
    struct Playback {
        double in_time{0.0};
        double out_time{0.0};
        double local_time_seconds{0.0};
        LayerTransitionSpec transition_in;
        LayerTransitionSpec transition_out;
    } playback;

    // Effects Pipeline
    std::vector<EvaluatedEffect> effects;
    
    // Matte Support
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::size_t> track_matte_layer_index;

    // Precompositions
    std::shared_ptr<EvaluatedCompositionState> nested_composition;

    // Compatibility Helpers (Read-only)
    const std::string& id() const { return identity.id.empty() ? identity.layer_id : identity.id; }
    const std::string& name() const { return identity.name; }
    LayerType type() const { return identity.type; }
};

struct EvaluatedCameraState {
    std::string id;
    math::Matrix3x3 view_matrix;
    math::Matrix3x3 projection_matrix;
    math::Matrix3x3 view_projection;
    float zoom{1.0f};
    bool active{true};
};

struct EvaluatedCompositionState {
    std::string composition_id;
    std::uint32_t width{1920};
    std::uint32_t height{1080};
    std::int64_t frame_number{0};
    double duration_seconds{0.0};
    double composition_time_seconds{0.0};
    std::vector<EvaluatedLayerState> layers;
    std::optional<EvaluatedCameraState> active_camera;
};

} // namespace tachyon::scene
