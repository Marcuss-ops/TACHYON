#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/core/renderer2d_surface_pool.h"

// For backward compatibility
using SurfacePool = tachyon::renderer2d::SurfacePool;
#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/renderer2d/backend/compute_backend.h"
#include "tachyon/renderer2d/color/color_management_system.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/text/fonts/core/font.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/text/content/subtitle.h"
#include "tachyon/runtime/execution/planning/quality_policy.h"

#include <array>
#include <vector>
#include <optional>
#include <string>
#include <memory>

namespace tachyon::media {
class MediaManager;
}

namespace tachyon::renderer3d {
class RayTracer;
}

namespace tachyon::renderer2d {

struct EvaluatorContext {
  std::vector<float> scalars;
  std::vector<Color> colors;
  std::vector<std::string> strings;
};

struct LayerContext {
  std::uint32_t index;
  EvaluatorContext* evaluator;
  const LayerSpec* layer;
  float layer_time_seconds;
  float parent_opacity;
  std::optional<std::uint32_t> parent_index;
  std::optional<std::string> parent_name;
  std::optional<std::string> track_matte_layer_id;
  std::optional<std::string> precomp_id;
  std::optional<std::uint32_t> precomp_index;
  std::optional<std::uint32_t> track_index;
  bool is_3d;
  bool is_adjustment_layer;
  bool visible;
  bool enabled;
  bool time_remap_enabled;
  float stroke_width_scale;
  float time_remap_offset_seconds;
  float time_remap_duration_seconds;
  float stroke_width;
  float start_time;
  float in_point;
  float out_point;
  float opacity;
  std::int64_t width;
  std::int64_t height;
  std::optional<ShapePathSpec> shape_path;
};

struct AccumulationBuffers {
    std::vector<float> r;
    std::vector<float> g;
    std::vector<float> b;
    std::vector<float> a;

    void resize(std::size_t size) {
        r.assign(size, 0.0f);
        g.assign(size, 0.0f);
        b.assign(size, 0.0f);
        a.assign(size, 0.0f);
    }
};

struct RenderContext2D {
    RenderContext2D();
    explicit RenderContext2D(std::shared_ptr<PrecompCache> cache);

    std::shared_ptr<SurfaceRGBA> framebuffer;
    std::shared_ptr<PrecompCache> precomp_cache;
    std::shared_ptr<SurfacePool> surface_pool;
    std::shared_ptr<EffectHost> effects;
    AccumulationBuffers accumulation_buffer;
    QualityPolicy policy;
    std::shared_ptr<class ::tachyon::renderer3d::RayTracer> ray_tracer;
    ColorManagementSystem cms;
    WorkingColorSpace working_color_space;
    int width{0};
    int height{0};

    // Text rendering state (formerly TextRenderConfig singleton)
    const ::tachyon::text::FontRegistry* font_registry = nullptr;
    const std::vector<::tachyon::text::SubtitleEntry>* subtitle_entries = nullptr;
    ::tachyon::media::MediaManager* media_manager = nullptr;

    // GPU/CPU compute backend. Null = use CPU implementations directly.
    // Set by QualityPolicy::build_render_context() or caller.
    // Non-owning if borrowed from a session-level backend; owning if created per-context.
    std::shared_ptr<ComputeBackend> compute_backend;
};

using RenderContext = RenderContext2D;

struct EvaluationResult {
  scene::EvaluatedCompositionState state;
  std::vector<LayerContext> layers;
  std::optional<std::string> error;
};

}  // namespace tachyon::renderer2d


