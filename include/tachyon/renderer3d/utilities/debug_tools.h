#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"
#include "tachyon/renderer3d/visibility/culling.h"
#include "tachyon/media/loading/mesh_asset.h"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <sstream>

namespace tachyon::renderer3d {

class RendererDebug {
public:
    struct VisualizerConfig {
        bool show_bounding_boxes{false};
        bool show_wireframe{false};
        bool show_normals{false};
        bool show_mesh_axes{false};
        bool show_light_positions{false};
        bool show_camera_info{false};
        bool show_node_hierarchy{false};
        bool show_texture_coords{false};
        bool show_collision_volumes{false};
        
        float bounding_box_color[3]{1.0f, 1.0f, 0.0f};
        float wireframe_color[3]{0.0f, 1.0f, 0.0f};
        float normal_color[3]{0.0f, 0.5f, 1.0f};
        float selected_color[3]{1.0f, 0.0f, 0.0f};
    };

    struct DebugOverlay {
        struct BoundingBoxDebug {
            std::string layer_id;
            SceneCulling::BoundingBox box;
            float color[3];
            bool is_selected{false};
        };

        struct WireframeDebug {
            std::string layer_id;
            std::vector<math::Vector3> vertices;
            std::vector<unsigned int> indices;
            float color[3];
        };

        struct NormalDebug {
            std::string layer_id;
            math::Vector3 position;
            math::Vector3 normal;
            float length{1.0f};
        };

        struct TextLabel {
            std::string text;
            math::Vector3 position;
            float color[3]{1.0f, 1.0f, 1.0f};
            float size{12.0f};
        };

        std::vector<BoundingBoxDebug> bounding_boxes;
        std::vector<WireframeDebug> wireframes;
        std::vector<NormalDebug> normals;
        std::vector<TextLabel> labels;
    };

    RendererDebug();

    void set_config(const VisualizerConfig& config);
    const VisualizerConfig& get_config() const { return config_; }

    DebugOverlay generate_overlay(const scene::EvaluatedCompositionState& state);

    void set_selected_layer(const std::string& layer_id);
    std::string get_selected_layer() const { return selected_layer_; }

    void set_display_mode(const std::string& mode);
    std::string get_display_mode() const { return display_mode_; }

    struct ProfilerData {
        struct Section {
            std::string name;
            double time_ms{0.0};
            int call_count{0};
            double min_time_ms{0.0};
            double max_time_ms{0.0};
            double avg_time_ms{0.0};
        };

        std::vector<Section> sections;
        std::unordered_map<std::string, std::size_t> section_indices;
        std::size_t current_section_{0};
        std::chrono::high_resolution_clock::time_point section_start_{};

        void begin_section(const std::string& name);
        void end_section(const std::string& name);
        void reset();

        std::string report() const;
        std::string report_json() const;
    };

    class ScopedTimer {
    public:
        ScopedTimer(ProfilerData& profiler, const std::string& section_name)
            : profiler_(profiler), section_name_(section_name) {
            profiler_.begin_section(section_name_);
        }
        ~ScopedTimer() {
            profiler_.end_section(section_name_);
        }

    private:
        ProfilerData& profiler_;
        std::string section_name_;
    };

    ProfilerData& get_profiler() { return profiler_; }
    const ProfilerData& get_profiler() const { return profiler_; }

    struct Diagnostics {
        int triangles_rendered{0};
        int rays_traced{0};
        int rays_per_pixel{0};
        int geometry_primitives{0};
        int material_samples{0};
        int texture_samples{0};
        int light_samples{0};
        int culled_objects{0};
        int visible_objects{0};
        double frame_time_ms{0.0};
        double render_time_ms{0.0};
        double build_time_ms{0.0};
        double trace_time_ms{0.0};
        double shade_time_ms{0.0};
        double denoise_time_ms{0.0};
        double memory_used_mb{0.0};
        int cache_hits{0};
        int cache_misses{0};
        float fps{0.0f};
    };

    void set_diagnostics(const Diagnostics& diag);
    const Diagnostics& get_diagnostics() const { return diagnostics_; }
    std::string diagnostics_report() const;

    static std::string format_memory(double mb);
    static std::string format_time(double ms);
    static std::string format_percentage(float value);

private:
    VisualizerConfig config_;
    std::string selected_layer_;
    std::string display_mode_;
    ProfilerData profiler_;
    Diagnostics diagnostics_;
};

class SceneInspector {
public:
    struct LayerInfo {
        std::string id;
        std::string name;
        scene::LayerType type;
        bool visible{false};
        bool is_3d{false};
        math::Vector3 world_position;
        math::Matrix4x4 world_matrix;
        SceneCulling::BoundingBox bounds;
    };

    struct LightInfo {
        std::string id;
        std::string type;
        math::Vector3 position;
        math::Vector3 direction;
        float intensity{0.0f};
        float attenuation_near{0.0f};
        float attenuation_far{0.0f};
    };

    struct CameraInfo {
        std::string id;
        math::Vector3 position;
        math::Vector3 point_of_interest;
        float fov{60.0f};
        float aperture{0.0f};
        float focus_distance{1000.0f};
    };

    struct MemoryInfo {
        std::size_t geometry_bytes{0};
        std::size_t texture_bytes{0};
        std::size_t buffer_bytes{0};
        std::size_t total_bytes{0};
    };

    static std::vector<LayerInfo> inspect_layers(const scene::EvaluatedCompositionState& state);
    static std::vector<LightInfo> inspect_lights(const scene::EvaluatedCompositionState& state);
    static CameraInfo inspect_camera(const scene::EvaluatedCameraState& state);
    static MemoryInfo estimate_memory(const scene::EvaluatedCompositionState& state);

    static std::string layer_report(const std::vector<LayerInfo>& layers);
    static std::string light_report(const std::vector<LightInfo>& lights);
    static std::string camera_report(const CameraInfo& camera);
    static std::string memory_report(const MemoryInfo& memory);
};

} // namespace tachyon::renderer3d

