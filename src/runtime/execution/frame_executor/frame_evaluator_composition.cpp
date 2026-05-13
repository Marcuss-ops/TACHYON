#include "frame_executor_internal.h"

#include <chrono>

namespace tachyon {

void evaluate_composition(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledComposition& comp,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const std::uint64_t composition_key,
    std::uint64_t node_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task,
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    (void)composition_key;
    (void)scene;
    (void)plan;
    (void)snapshot;
    (void)context;
    (void)main_frame_key;
    (void)main_frame_time;

    const auto timing_start = std::chrono::high_resolution_clock::now();
    auto record_timing = [&]() {
        if (!context.diagnostics) {
            return;
        }
        const auto timing_end = std::chrono::high_resolution_clock::now();
        context.diagnostics->timings.push_back(TimingSample{
            "composition",
            std::to_string(comp.node.node_id),
            std::chrono::duration<double, std::milli>(timing_end - timing_start).count()
        });
    };

    if (executor.m_cache.lookup_composition(node_key)) {
        if (context.diagnostics) context.diagnostics->composition_hits++;
        record_timing();
        return;
    }

    if (context.diagnostics) {
        context.diagnostics->composition_misses++;
        context.diagnostics->compositions_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->width = comp.width;
    state->height = comp.height;
    state->frame_rate.numerator = comp.fps;
    state->frame_rate.denominator = 1;
    state->frame_number = task.frame_number;
    state->composition_time_seconds = frame_time_seconds;

    state->layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        const std::uint64_t layer_key = build_node_key(frame_key, layer.node);
        auto cached_layer = executor.m_cache.lookup_layer(layer_key);
        if (cached_layer) {
            state->layers.push_back(*cached_layer);
        }
    }

    for (const auto& layer_state : state->layers) {
        if (layer_state.type != scene::LayerType::Camera || !layer_state.enabled || !layer_state.active) {
            continue;
        }

        state->camera.available = true;
        state->camera.layer_id = layer_state.id;
        state->camera.name = layer_state.name.empty() ? "Camera" : layer_state.name;
        state->camera.camera_type = layer_state.camera_type;
        const math::Vector3 synth_camera_position{
            static_cast<float>(comp.width) * 0.5f,
            static_cast<float>(comp.height) * 0.5f,
            -900.0f
        };
        const math::Vector3 synth_camera_target{
            static_cast<float>(comp.width) * 0.5f,
            static_cast<float>(comp.height) * 0.5f,
            0.0f
        };
        state->camera.position = synth_camera_position;
        state->camera.point_of_interest = synth_camera_target;
        state->camera.up = {0.0f, 1.0f, 0.0f};
        state->camera.roll = 0.0f;
        state->camera.zoom = layer_state.zoom;
        state->camera.aspect = comp.height > 0 ? static_cast<float>(comp.width) / static_cast<float>(comp.height) : 1.777778f;
        state->camera.fov_y_rad = 2.0f * std::atan(static_cast<float>(comp.height) / (2.0f * std::max(layer_state.zoom, 1.0f)));
        state->camera.focus_distance = layer_state.camera_focus_distance.value_or(
            std::max(1.0f, (state->camera.point_of_interest - state->camera.position).length()));
        state->camera.aperture = layer_state.camera_aperture.value_or(0.0f);
        state->camera.dof_enabled = layer_state.camera_aperture.has_value();
        state->camera.previous_world_matrix = layer_state.previous_world_matrix;
        state->camera.camera.transform.position = state->camera.position;
        state->camera.camera.transform.rotation = math::Quaternion::identity();
        state->camera.camera.transform.scale = {1.0f, 1.0f, 1.0f};
        state->camera.camera.target_position = state->camera.point_of_interest;
        state->camera.camera.up = state->camera.up;
        state->camera.camera.roll_deg = state->camera.roll;
        state->camera.camera.use_target = true;
        state->camera.camera.fov_y_rad = state->camera.fov_y_rad;
        state->camera.camera.aspect = state->camera.aspect;
        state->camera.camera.near_z = state->camera.near_clip;
        state->camera.camera.far_z = state->camera.far_clip;
        state->camera.view_matrix = state->camera.camera.get_view_matrix();
        state->camera.projection_matrix = state->camera.camera.get_projection_matrix();
        break;
    }

    if (state->lights.empty()) {
        for (const auto& layer_state : state->layers) {
            if (layer_state.type != scene::LayerType::Light || !layer_state.enabled || !layer_state.active) {
                continue;
            }

            scene::EvaluatedLightState light;
            light.layer_id = layer_state.id;
            light.type = "point";
            light.position = layer_state.world_position3;
            light.direction = layer_state.world_matrix.transform_vector({0.0f, 0.0f, -1.0f}).normalized();
            light.color = math::Vector3{
                layer_state.fill_color.r / 255.0f,
                layer_state.fill_color.g / 255.0f,
                layer_state.fill_color.b / 255.0f
            };
            light.intensity = 1.0f;
            light.attenuation_near = 0.0f;
            light.attenuation_far = 2000.0f;
            light.cone_angle = 90.0f;
            light.cone_feather = 0.0f;
            light.casts_shadows = false;
            light.shadow_darkness = 0.0f;
            light.shadow_radius = 0.0f;
            light.falloff_type = "inverse_square";
            state->lights.push_back(std::move(light));
        }
    }

    executor.m_cache.store_composition(node_key, std::move(state));
    record_timing();
}

} // namespace tachyon
