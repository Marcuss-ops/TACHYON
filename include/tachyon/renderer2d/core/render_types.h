#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/matrix4x4.h"
#include <vector>
#include <string>
#include <optional>
#include <cstdint>

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

struct PrimitiveVertex {
    math::Vector3 position;
    math::Vector3 normal;
    math::Vector2 uv;
};

struct PrimitiveMaterial {
    math::Vector3 base_color_factor{1.0f, 1.0f, 1.0f};
    float metallic_factor{0.0f};
    float roughness_factor{0.5f};
    std::optional<std::uint32_t> base_color_texture_idx;
};

struct HDRTextureData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    std::vector<float> data;
};

struct PrimitiveSubMesh {
    std::vector<PrimitiveVertex> vertices;
    std::vector<std::uint32_t> indices;
    PrimitiveMaterial material;
    math::Matrix4x4 transform{math::Matrix4x4::identity()};
};

struct PrimitiveMesh {
    std::string path;
    std::vector<PrimitiveSubMesh> sub_meshes;
    struct TextureData {
        int width;
        int height;
        int channels;
        std::vector<std::uint8_t> data;
    };
    std::vector<TextureData> textures;
};

} // namespace tachyon::renderer2d
