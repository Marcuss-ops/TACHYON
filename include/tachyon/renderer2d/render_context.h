#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/precomp_cache.h"
#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/runtime/quality_policy.h"

#include <array>
#include <vector>
#include <optional>
#include <string>
#include <memory>

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

struct RenderContext {
    RenderContext(std::shared_ptr<PrecompCache> cache = std::make_shared<PrecompCache>()) 
        : precomp_cache(std::move(cache)) {}

    std::shared_ptr<Framebuffer> framebuffer;
    std::shared_ptr<PrecompCache> precomp_cache;
    std::shared_ptr<EffectHost> effects;
    AccumulationBuffers accumulation_buffer;
    QualityPolicy policy;
    std::shared_ptr<renderer3d::RayTracer> ray_tracer;
};

struct EvaluationResult {
  scene::EvaluatedCompositionState state;
  std::vector<LayerContext> layers;
  std::optional<std::string> error;
};

}  // namespace tachyon::renderer2d
