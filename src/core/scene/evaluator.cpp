#include "tachyon/core/scene/evaluator_math_forward.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/timeline/time.h"
#include "tachyon/renderer2d/expressions/expression_evaluator.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/audio/audio_sampling.h"
#include "tachyon/text/subtitle.h"

#include "tachyon/core/scene/evaluator_internal.h"
#include "tachyon/core/scene/evaluator_utils.h"
#include "tachyon/core/scene/evaluator_layer.h"
#include "tachyon/core/scene/evaluator_composition.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace tachyon {
namespace scene {

EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    media::MediaManager* media) {
    CompositionSpec composition;
    composition.id = "standalone";
    composition.name = "standalone";
    
    EvaluationContext context{
        nullptr,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        {std::nullopt}, // One layer cache
        {false},        // One layer visiting
        {},
        audio_analyzer,
        {},
        {},
        media
    };
    context.layer_indices.emplace(layer.id, 0);

    return make_layer_state(context, layer, 0);
}

EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
) {
    (void)frame_number;
    (void)composition_time_seconds;
    EvaluatedCameraState evaluated;
    evaluated.camera.aspect = composition.height > 0
        ? static_cast<float>(static_cast<double>(composition.width) / static_cast<double>(composition.height))
        : 1.0f;

    // Find the first active camera from top to bottom
    const LayerSpec* camera_spec = nullptr;
    const EvaluatedLayerState* camera_layer = nullptr;

    for (std::size_t i = 0; i < composition.layers.size(); ++i) {
        const auto& spec = composition.layers[i];
        if (spec.type != "camera") continue;
        
        // Find corresponding evaluated layer
        for (const auto& el : layers) {
            if (el.id == spec.id && el.active) {
                camera_spec = &spec;
                camera_layer = &el;
                break;
            }
        }
        if (camera_spec) break;
    }

    if (camera_layer && camera_spec) {
        evaluated.available = true;
        evaluated.layer_id = camera_layer->id;
        evaluated.position = camera_layer->world_position3;
        
        // AE Camera Logic
        evaluated.is_two_node = camera_spec->is_two_node;
        
        if (evaluated.is_two_node) {
            const std::uint64_t cam_seed = hash_combine(stable_string_hash(camera_layer->id), stable_string_hash("camera"));
            evaluated.point_of_interest = sample_vector3(camera_spec->point_of_interest, {0,0,0}, camera_layer->child_time_seconds, nullptr, hash_combine(cam_seed, stable_string_hash("poi")));
        }

        // DOF and optics
        evaluated.dof_enabled = camera_spec->dof_enabled;
        const std::uint64_t opt_seed = stable_string_hash(camera_layer->id);
        evaluated.focus_distance = static_cast<float>(sample_scalar(camera_spec->focus_distance, 1000.0, camera_layer->child_time_seconds, nullptr, hash_combine(opt_seed, stable_string_hash("focus"))));
        evaluated.aperture = static_cast<float>(sample_scalar(camera_spec->aperture, 0.0, camera_layer->child_time_seconds, nullptr, hash_combine(opt_seed, stable_string_hash("aperture"))));
        evaluated.blur_level = static_cast<float>(sample_scalar(camera_spec->blur_level, 100.0, camera_layer->child_time_seconds, nullptr, hash_combine(opt_seed, stable_string_hash("blur"))));
        evaluated.aperture_blades = static_cast<int>(sample_scalar(camera_spec->aperture_blades, 0.0, camera_layer->child_time_seconds, nullptr, hash_combine(opt_seed, stable_string_hash("blades"))));
        evaluated.aperture_rotation = static_cast<float>(sample_scalar(camera_spec->aperture_rotation, 0.0, camera_layer->child_time_seconds, nullptr, hash_combine(opt_seed, stable_string_hash("aperture_rot"))));

        // Use world matrix for transform
        if (evaluated.is_two_node) {
            const math::Quaternion look_rot = math::Quaternion::look_at(camera_layer->world_position3, evaluated.point_of_interest, {0, 1, 0});
            evaluated.camera.transform.compose_trs(
                camera_layer->world_position3,
                look_rot,
                math::Vector3::one()
            );
        } else {
            // Extract rotation from world_matrix
            const math::Matrix4x4& m = camera_layer->world_matrix;
            const math::Vector3 right{m[0], m[1], m[2]};
            const math::Vector3 up{m[4], m[5], m[6]};
            const math::Vector3 forward{-m[8], -m[9], -m[10]}; // Invert Z as per convention
            
            evaluated.camera.transform.compose_trs(
                camera_layer->world_position3,
                math::Quaternion::look_at({0,0,0}, forward, up),
                math::Vector3::one()
            );
        }
        // Better: extract rotation from world_matrix
        // For now, world_matrix is the truth.
    }

    return evaluated;
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {
    const auto frame = timeline::sample_frame(composition.frame_rate, frame_number);
    return evaluate_composition_internal(nullptr, composition, frame.frame_number, frame.composition_time_seconds, {}, audio_analyzer, vars, media);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media) {
    const std::int64_t frame_number = static_cast<std::int64_t>(std::llround(composition_time_seconds * static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator)));
    return evaluate_composition_internal(nullptr, composition, frame_number, composition_time_seconds, {}, audio_analyzer, vars, media);
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media
) {
    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            const auto frame = timeline::sample_frame(composition.frame_rate, frame_number);
            return evaluate_composition_internal(&scene, composition, frame.frame_number, frame.composition_time_seconds, {}, audio_analyzer, vars, media);
        }
    }
    return std::nullopt;
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars,
    media::MediaManager* media
) {
    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            const std::int64_t frame_number = static_cast<std::int64_t>(std::llround(composition_time_seconds * static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator)));
            return evaluate_composition_internal(&scene, composition, frame_number, composition_time_seconds, {}, audio_analyzer, vars, media);
        }
    }
    return std::nullopt;
}

EvaluatedLayerState::EvaluatedLayerState(const EvaluatedLayerState& other) {
    layer_index = other.layer_index;
    id = other.id;
    name = other.name;
    type = other.type;
    blend_mode = other.blend_mode;
    enabled = other.enabled;
    active = other.active;
    visible = other.visible;
    is_3d = other.is_3d;
    is_adjustment_layer = other.is_adjustment_layer;
    local_time_seconds = other.local_time_seconds;
    child_time_seconds = other.child_time_seconds;
    opacity = other.opacity;
    local_transform = other.local_transform;
    world_matrix = other.world_matrix;
    orientation_xyz_deg = other.orientation_xyz_deg;
    anchor_point_3d = other.anchor_point_3d;
    scale_3d = other.scale_3d;
    world_position3 = other.world_position3;
    world_normal = other.world_normal;
    material = other.material;
    extrusion_depth = other.extrusion_depth;
    bevel_size = other.bevel_size;
    width = other.width;
    height = other.height;
    text_content = other.text_content;
    font_id = other.font_id;
    font_size = other.font_size;
    text_alignment = other.text_alignment;
    fill_color = other.fill_color;
    stroke_color = other.stroke_color;
    stroke_width = other.stroke_width;
    line_cap = other.line_cap;
    line_join = other.line_join;
    miter_limit = other.miter_limit;
    shape_path = other.shape_path;
    gradient_fill = other.gradient_fill;
    gradient_stroke = other.gradient_stroke;
    effects = other.effects;
    subtitle_path = other.subtitle_path;
    subtitle_outline_color = other.subtitle_outline_color;
    subtitle_outline_width = other.subtitle_outline_width;
    track_matte_type = other.track_matte_type;
    track_matte_layer_index = other.track_matte_layer_index;
    precomp_id = other.precomp_id;
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    }
    asset_id = other.asset_id;
    asset_path = other.asset_path;
}

EvaluatedLayerState& EvaluatedLayerState::operator=(const EvaluatedLayerState& other) {
    if (this == &other) return *this;
    layer_index = other.layer_index;
    id = other.id;
    name = other.name;
    type = other.type;
    blend_mode = other.blend_mode;
    enabled = other.enabled;
    active = other.active;
    visible = other.visible;
    is_3d = other.is_3d;
    is_adjustment_layer = other.is_adjustment_layer;
    local_time_seconds = other.local_time_seconds;
    child_time_seconds = other.child_time_seconds;
    opacity = other.opacity;
    local_transform = other.local_transform;
    world_matrix = other.world_matrix;
    orientation_xyz_deg = other.orientation_xyz_deg;
    anchor_point_3d = other.anchor_point_3d;
    scale_3d = other.scale_3d;
    world_position3 = other.world_position3;
    world_normal = other.world_normal;
    material = other.material;
    extrusion_depth = other.extrusion_depth;
    bevel_size = other.bevel_size;
    width = other.width;
    height = other.height;
    text_content = other.text_content;
    font_id = other.font_id;
    font_size = other.font_size;
    text_alignment = other.text_alignment;
    fill_color = other.fill_color;
    stroke_color = other.stroke_color;
    stroke_width = other.stroke_width;
    line_cap = other.line_cap;
    line_join = other.line_join;
    miter_limit = other.miter_limit;
    shape_path = other.shape_path;
    gradient_fill = other.gradient_fill;
    gradient_stroke = other.gradient_stroke;
    effects = other.effects;
    subtitle_path = other.subtitle_path;
    subtitle_outline_color = other.subtitle_outline_color;
    subtitle_outline_width = other.subtitle_outline_width;
    track_matte_type = other.track_matte_type;
    track_matte_layer_index = other.track_matte_layer_index;
    precomp_id = other.precomp_id;
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    } else {
        nested_composition.reset();
    }
    asset_id = other.asset_id;
    asset_path = other.asset_path;
    return *this;
}

} // namespace scene
} // namespace tachyon
