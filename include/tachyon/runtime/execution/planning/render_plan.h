#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/timeline/time_remap.h"

#include "tachyon/runtime/execution/planning/quality_policy.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

struct CompositionSummary {
    std::string id;
    std::string name;
    std::int64_t width{0};
    std::int64_t height{0};
    double duration{0.0};
    FrameRate frame_rate;
    std::optional<BackgroundSpec> background;
    std::size_t layer_count{0};
    std::size_t solid_layer_count{0};
    std::size_t shape_layer_count{0};
    std::size_t mask_layer_count{0};
    std::size_t image_layer_count{0};
    std::size_t text_layer_count{0};
    std::size_t precomp_layer_count{0};
    std::size_t track_matte_layer_count{0};
};

enum class MotionBlurMode {
    CameraOnly,
    SceneTemporal
};

struct RenderPlan {
    std::string job_id;
    std::string scene_ref;
    std::string composition_target;
    CompositionSummary composition;
    FrameRange frame_range;
    OutputContract output;
    std::string quality_tier{"high"};
    QualityPolicy quality_policy;
    std::string compositing_alpha_mode{"premultiplied"};
    std::string working_space{"linear_rec709"};
    
    // Motion Blur
    bool motion_blur_enabled{false};
    MotionBlurMode motion_blur_mode{MotionBlurMode::CameraOnly};
    std::int64_t motion_blur_samples{0};
    double motion_blur_shutter_angle{180.0};
    double motion_blur_shutter_phase{-90.0};
    std::string motion_blur_curve{"box"};

    // Time Remap & Frame Blend
    std::optional<timeline::TimeRemapCurve> time_remap_curve;
    timeline::FrameBlendMode frame_blend_mode{timeline::FrameBlendMode::Linear};
    
    std::string seed_policy_mode;
    std::string compatibility_mode;
    const SceneSpec* scene_spec{nullptr};
    
    std::uint64_t scene_hash{0};
    int contract_version{1};
    bool proxy_enabled{false};
    
    struct OCIOConfig {
        std::string config_path;
        std::string display;
        std::string view;
        std::string look;
    } ocio;

    struct DoFParams {
        bool enabled{false};
        double aperture{0.0};
        double focus_distance{0.0};
        double focal_length{0.0};
    } dof;

    std::unordered_map<std::string, double> variables;
    std::unordered_map<std::string, std::string> string_variables;
    std::unordered_map<std::string, RenderJob::LayerOverride> layer_overrides;
};

/**
 * @brief Simple wrapper for a cache lookup key.
 */
struct CacheKey {
    std::uint64_t hash{0};
    std::string value;

    CacheKey() = default;
    CacheKey(std::string v) : value(std::move(v)) {}
    CacheKey(std::uint64_t h, std::string v) : hash(h), value(std::move(v)) {}
};

using FrameCacheKey = CacheKey;

struct FrameCacheEntry {
    FrameCacheKey key;
    std::string label;
};

struct CachedFrame {
    FrameCacheEntry entry;
    std::uint64_t scene_hash{0};
    renderer2d::Framebuffer frame{1, 1};
    std::vector<std::string> invalidates_when_changed;
};

/**
 * @brief Categorization of render steps for the execution plan.
 */
enum class RenderStepKind {
    Unknown,
    Precomp,
    Layer,
    Effect,
    Rasterize2DFrame,
    Output
};

/**
 * @brief A single execution step in the render session.
 */
struct RenderStep {
    std::string id;
    RenderStepKind kind{RenderStepKind::Unknown};
    std::string label;
    std::vector<std::string> dependencies;
    std::optional<std::int64_t> frame_number;
};

/**
 * @brief Represents a single frame to be rendered.
 */
struct FrameRenderTask {
    std::int64_t frame_number{0};
    double time_seconds{0.0};
    std::vector<std::string> layer_filters;
    CacheKey cache_key;
    bool cacheable{true};
    std::vector<std::string> invalidates_when_changed;
    std::optional<std::uint64_t> subframe_index;
    std::optional<std::uint64_t> motion_blur_sample_index;
};

/**
 * @brief High-level execution plan for a render session.
 */
struct RenderExecutionPlan {
    RenderPlan render_plan;
    std::vector<FrameRenderTask> frame_tasks;
    std::vector<RenderStep> steps;
    std::size_t resolved_asset_count{0};
};

std::string render_step_kind_string(RenderStepKind kind);

ResolutionResult<RenderPlan> build_render_plan(const SceneSpec& scene, const RenderJob& job);
ResolutionResult<RenderExecutionPlan> build_render_execution_plan(const RenderPlan& plan, std::size_t assets_count);

[[nodiscard]] FrameCacheKey build_frame_cache_key(const RenderPlan& plan, std::int64_t frame_number);
[[nodiscard]] bool frame_cache_entry_matches(const FrameCacheEntry& entry, const FrameCacheKey& key);

} // namespace tachyon
